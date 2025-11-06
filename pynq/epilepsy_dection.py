# -*- coding: utf-8 -*-
"""
Loopback controller (PYNQ) — original(10s) / TDATA[15:0] / state
+ LED alarm via MMIO (CH1 only)
+ Adjustable DMA pacing (DMA_SPEED_MULT), LED locked to DMA rate.
"""

import os, sys, time, math, threading, queue
import numpy as np
import matplotlib.pyplot as plt

from pynq import Overlay, allocate, MMIO
from pynq.lib.dma import DMA

# ===================== CONFIG =====================
BIT_PATH   = "/home/xilinx/jupyter_notebooks/overlays/epilepsy_detection/detection_final.bit"
DMA_NAME   = "axi_dma_0"
INPUT_FILE = "/home/xilinx/jupyter_notebooks/signal/signal.txt"

FS_HZ      = 1024
SAVE_PNG   = "/home/xilinx/jupyter_notebooks/loopback_plot.png"

# LED (AXI GPIO CH1 via MMIO)
LED_IP_NAME      = "axi_gpio_led"         # 固定用这个名字
LED_ACTIVE_HIGH  = True                   # 固定为高电平点亮
LED_MASK         = 0xF
LED_CLEAR_AT_END = True

# 仅用于兼容旧代码；实际不再使用该变量控制 LED 速度
LED_SPEED_MULT   = 20.0                   # <ignored> LED 现在跟随 DMA_SPEED_MULT

# Streaming
CHUNK_SAMPLES    = 2048                   # 每片样本数(≈2 s at 1×)
PRINT_PROGRESS   = True                   # 打印分片进度
LED_LOG_INTERVAL_S = 1.0                  # LED 播放线程日志间隔(s)

# === 新增：DMA 发送速率倍率（唯一节拍源） ===
DMA_SPEED_MULT   = 10.0                    # 1.0=真实速率；20.0=20倍速（900s --> ~45s）
# ==================================================

def load_int16_series(path):
    if path.lower().endswith(".npy"):
        x = np.load(path).astype(np.int16, copy=False)
    else:
        x = np.loadtxt(path, dtype=np.int16)
    if x.ndim != 1:
        raise ValueError("Input must be 1-D int16 series")
    return x

class LedCh1:
    def __init__(self, ol, ip_name, active_high=True, mask=0xF):
        ip = ol.ip_dict[ip_name]
        self.mm = MMIO(ip['phys_addr'], ip['addr_range'])
        self.active_high = bool(active_high)
        self.mask = int(mask) & 0xFFFFFFFF
        self.OFF_DATA, self.OFF_TRI = 0x0, 0x4
        self.mm.write(self.OFF_TRI, 0x00000000)  # CH1 输出
        self.write(0)
    def write(self, value):
        v = int(value) & self.mask
        if not self.active_high:
            v = (~v) & self.mask
        self.mm.write(self.OFF_DATA, v)
    def blink_test(self):
        print(f"[LED] blink test on {LED_IP_NAME} CH1, active-high, mask=0x{self.mask:X}")
        self.write(self.mask); time.sleep(0.3)
        for i in range(4): self.write(1<<i); time.sleep(0.12)
        self.write(0); time.sleep(0.2)

def led_player(q: "queue.Queue[np.ndarray]", fs_state_hz: float, speed_mult: float,
               led: LedCh1, clear_at_end=False, log_interval_s=1.0):
    """按 (状态采样率 × DMA_SPEED_MULT) 逐点写 LED；当队列无数据时阻塞等待。"""
    if fs_state_hz <= 0: return
    fs_play = max(1.0, fs_state_hz * max(0.01, float(speed_mult)))
    period = 1.0 / fs_play
    played = 0
    next_log = time.perf_counter() + max(0.2, float(log_interval_s))
    try:
        t_next = time.perf_counter()
        while True:
            arr = q.get()            # 阻塞式取片（确保无丢片）
            if arr is None:
                break
            for v in arr:
                led.write(int(v) & led.mask)
                played += 1
                t_next += period
                dt = t_next - time.perf_counter()
                if dt > 0:
                    time.sleep(dt)
                if time.perf_counter() >= next_log:
                    try:
                        qsz = q.qsize()
                    except NotImplementedError:
                        qsz = -1
                    print(f"[LED] @{fs_play:.1f}Hz (locked to DMA x{speed_mult:.2f}), "
                          f"played={played}, last=0x{int(v)&0xF:X}, queue={qsz}")
                    next_log += log_interval_s
    finally:
        if clear_at_end:
            led.write(0)

def main():
    print("Loading bitstream...")
    ol = Overlay(BIT_PATH)
    dma = DMA(ol.ip_dict[DMA_NAME])
    led = LedCh1(ol, LED_IP_NAME, active_high=LED_ACTIVE_HIGH, mask=LED_MASK)
    led.blink_test()

    print("Loading input LFP (int16) from:", INPUT_FILE)
    x = load_int16_series(INPUT_FILE)
    N = x.size
    print(f"[main] Input length: {N} @ {FS_HZ} Hz (~{N/FS_HZ:.2f} s)")
    print(f"[pace] DMA_SPEED_MULT={DMA_SPEED_MULT:.2f} (1.0=real-time). "
          f"LED speed is locked to DMA.")

    # 片缓冲
    tx = allocate(shape=(CHUNK_SAMPLES,), dtype=np.int16)
    rx = allocate(shape=(CHUNK_SAMPLES,), dtype=np.uint32)

    # LED 播放线程（估计完状态采样率后启动；速度=状态采样率×DMA_SPEED_MULT）
    state_q = queue.Queue(maxsize=8)
    fs_state_est, player_thread = None, None

    low16_all, state_all = [], []
    total_chunks = (N + CHUNK_SAMPLES - 1) // CHUNK_SAMPLES
    processed = 0

    # 统一节拍：按 DMA_SPEED_MULT 生成“计划起始时间”
    t0 = time.perf_counter()
    t_dma_next = t0

    for idx, off in enumerate(range(0, N, CHUNK_SAMPLES), start=1):
        chunk = x[off:off+CHUNK_SAMPLES]
        ns = chunk.size
        tx[:ns] = chunk
        if ns < CHUNK_SAMPLES: tx[ns:] = 0
        tx.flush()

        # 计划本片的墙钟时长： (ns/FS_HZ)/DMA_SPEED_MULT
        plan_inc = (ns / float(FS_HZ)) / max(1e-6, float(DMA_SPEED_MULT))
        now = time.perf_counter()
        if now < t_dma_next:
            time.sleep(t_dma_next - now)

        # 单片 DMA
        t_start = time.perf_counter()
        dma.recvchannel.transfer(rx)
        dma.sendchannel.transfer(tx)
        dma.sendchannel.wait(); dma.recvchannel.wait()
        rx.invalidate()
        t_end = time.perf_counter()

        # 解析本片（原生状态，不扩展）
        u32 = rx.view(np.uint32)
        if np.any(u32[:ns]):
            last = int(np.flatnonzero(u32[:ns])[-1])
            valid_len = last + 1
        else:
            valid_len = 0

        low16_raw = (u32[:valid_len] & 0xFFFF).astype(np.int16)
        state_raw = ((u32[:valid_len] >> 16) & 0xF).astype(np.int16)

        # 首片估计状态采样率，并启动 LED（速度=状态采样率×DMA_SPEED_MULT）
        if fs_state_est is None:
            fs_state_est = FS_HZ * (valid_len / float(ns)) if valid_len > 0 else FS_HZ
            fs_state_est = 256.0 if abs(fs_state_est-256)<abs(fs_state_est-1024) else 1024.0
            print(f"[state] estimated fs_state ≈ {fs_state_est:.1f} Hz; "
                  f"LED locked to DMA → target {fs_state_est*DMA_SPEED_MULT:.1f} Hz")
            player_thread = threading.Thread(
                target=led_player,
                args=(state_q, fs_state_est, DMA_SPEED_MULT, led, LED_CLEAR_AT_END, LED_LOG_INTERVAL_S),
                daemon=True
            )
            player_thread.start()
            print("[LED] player thread started.")

        # 送入 LED 队列（阻塞 put，确保无丢片，与 DMA 同步）
        if state_raw.size:
            state_q.put(state_raw.copy())

        # 累积绘图数据（扩展到输入采样率用于第二/三幅图）
        if valid_len == ns:
            low16_all.append(low16_raw); state_all.append(state_raw)
        elif valid_len > 0:
            rpt = ns // valid_len
            low16_all.append(np.repeat(low16_raw, rpt)[:ns])
            state_all.append(np.repeat(state_raw, rpt)[:ns])
        else:
            low16_all.append(np.zeros(ns, dtype=np.int16))
            state_all.append(np.zeros(ns, dtype=np.int16))

        processed += ns
        if PRINT_PROGRESS:
            dma_ms = (t_end - t_start) * 1e3
            plan_ms = plan_inc * 1e3
            elapsed = time.perf_counter() - t0
            pct = min(100.0, 100.0 * processed / N)
            qsz = state_q.qsize() if fs_state_est is not None else 0
            print(f"[chunk {idx}/{total_chunks}] ns={ns}, valid={valid_len}, "
                  f"plan={plan_ms:.1f}ms, DMA={dma_ms:.1f}ms, q={qsz}, progress={pct:.1f}%, "
                  f"elapsed={elapsed:.1f}s")

        # 推进下一片的“计划起始时间”
        t_dma_next += plan_inc

    # 结束 LED 播放
    if player_thread is not None:
        state_q.put(None)
        time.sleep(0.05)

    # ===== 拼接整段用于绘图 =====
    low16_full = np.concatenate(low16_all)[:N]
    state_full = np.concatenate(state_all)[:N]

    # 统计状态直方图
    if state_full.size > 0:
        uniq, cnt = np.unique(state_full, return_counts=True)
        print(f"[state] histogram -> {dict(zip(uniq.tolist(), cnt.tolist()))}")

    # ---------- Plot ----------
    t_full = np.arange(N, dtype=np.float64) / FS_HZ
    first_sec = min(N, int(10 * FS_HZ))
    t_10s = t_full[:first_sec]
    original_10s = x[:first_sec].astype(np.int16)

    fig, axes = plt.subplots(3, 1, figsize=(12, 7), sharex=False)

    axes[0].plot(t_10s, original_10s, lw=0.8)
    axes[0].set_title("Original (first 10 s)")
    axes[0].set_ylabel("Amplitude (int16)")
    axes[0].set_xlim(0, t_10s[-1] if first_sec > 0 else 10)

    axes[1].plot(t_full, low16_full, lw=0.8)
    axes[1].set_title("PL output (TDATA[15:0], full)")
    axes[1].set_ylabel("Amplitude (int16)")

    axes[2].step(t_full, state_full, where="post", lw=0.8)
    axes[2].set_title("State (0x1/0x2/0x4/0x8, full)")
    axes[2].set_ylabel("State code")
    axes[2].set_xlabel("Time (s)")

    fig.tight_layout()
    if SAVE_PNG:
        plt.savefig(SAVE_PNG, dpi=120)
        print("Saved plot to:", SAVE_PNG)
    plt.show()

if __name__ == "__main__":
    main()

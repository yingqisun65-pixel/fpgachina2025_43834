## 1.功能概述

- 从文件读取模拟 LFP（int16，一维序列），通过 AXI DMA 送入 PL 算法 IP。
- 采回 IP 输出的 TDATA[15:0]（低 16 位，视作“滤波/处理后”信号）与 状态位（TDATA[19:16]，四位热码 0x1/0x2/0x4/0x8）。
- 用 MMIO 控制 AXI GPIO（CH1），使 LED 实时跟随状态 变化。
- 可调 DMA 发送节奏倍率（DMA_SPEED_MULT）：1.0 为真实速率，20.0 为 20 倍速演示。
- 结束后在一张图里画出三条曲线：原始（前 10 s）/ TDATA[15:0]（全长）/ State（全长）。
- 仅搬运你的输入数据，不叠加任何软件滤波。

## 2.目录结构建议

project/  
├─ pynq/  
│  └─ loopback_controller.py     # 本仓库脚本（或命名为你的文件名）  
├─ overlays/  
│  └─ epilepsy_detection/  
│     ├─ detection_final.bit  
│     └─ detection_final.hwh     # 推荐与 .bit 同目录同名  
└─ signal/  
   └─ signal.txt                 # 一行一个 int16 样本（或 .npy）  

   
## 3. 运行环境

- 硬件：Zynq（例：Z-7020）+  PL 算法 IP（AXIS 接口），AXI GPIO 接 4-bit LED（CH1 输出）。
- 软件：PYNQ 镜像（带 pynq、numpy、matplotlib），Vivado 已生成 .bit/.hwh。
- Jupyter 推荐；也可在板子上 python3 直跑（只要有 PYNQ Python 环境）。

## 4. 参数说明

| 参数名                  | 说明                                                | 默认值                                                                              |
| -------------------- | ------------------------------------------------- | -------------------------------------------------------------------------------- |
| `BIT_PATH`           | Overlay 位流路径（建议与 `.hwh` 同名同目录）                    | `/home/xilinx/jupyter_notebooks/overlays/epilepsy_detection/detection_final.bit` |
| `DMA_NAME`           | 工程里的 DMA IP 名称（`ol.ip_dict` 中的键）                  | `axi_dma_0`                                                                      |
| `INPUT_FILE`         | 信号文件（`.txt` 一行一个 `int16` 或 `.npy` 的 `int16` 一维数组） | `/home/xilinx/jupyter_notebooks/signal/signal.txt`                               |
| `FS_HZ`              | 采样率（真实采样率，用于时间轴与节拍基准）                             | `1024`                                                                           |
| `LED_IP_NAME`        | LED 的 AXI GPIO IP 名称（`ol.ip_dict` 键）              | `axi_gpio_led`                                                                   |
| `LED_ACTIVE_HIGH`    | LED 极性（固定为高电平点亮）                                  | `True`                                                                           |
| `LED_MASK`           | LED 位掩码（4 盏灯 `0xF`）                               | `0xF`                                                                            |
| `LED_CLEAR_AT_END`   | 播放结束是否清灯                                          | `True`                                                                           |
| `CHUNK_SAMPLES`      | 每次 DMA 发送的样本数（分片大小，越小首帧越快）                        | `2048`                                                                           |
| `DMA_SPEED_MULT`     | **DMA 节奏倍率**：`1.0=真实速率`；`20.0=20×`                | `10.0`                                                                           |
| `PRINT_PROGRESS`     | 是否打印分片进度                                          | `True`                                                                           |
| `LED_LOG_INTERVAL_S` | LED 线程进度日志周期（秒）                                   | `1.0`                                                                            |



## 5. 原理简述

- 分片发送：将长度为 CHUNK_SAMPLES 的片段按 计划时间 ((ns/FS_HZ)/DMA_SPEED_MULT) 依次送入 DMA；若 CPU 过快，会 sleep 等到计划时刻，以维持整体节拍。
- 采回解析：每个分片回读 32-bit word 流，取 低 16 位为“滤波后信号”，高 4 位（[19:16]）为状态码。
- 时轴对齐：绘图时将状态序列扩展到 FS_HZ 以与原始信号对齐（仅用于画图；LED 播放使用原生状态点）。
- LED 同步：LED 线程按 状态采样率 × DMA_SPEED_MULT 定时逐点写入，保证与 DMA 播放倍率一致。

## 6. 常见问题 & 排错

### LED 不亮

确认 LED_IP_NAME 与 BD 中 AXI GPIO 的 实例名一致，且用 CH1。
如果你的硬件是低电平点亮，将 LED_ACTIVE_HIGH = False。
自检会先亮一遍，若自检不亮，检查连线/约束/电源。

### 脚本很快跑完

DMA_SPEED_MULT 设置太大就是“加速播”，比如 20× 会把 15 分钟数据压到 ~45 秒。
想按真实时间播，设为 1.0。

### 一直提示 queue full, dropping one chunk（如果你用过旧版）

当前版本 LED 线程采用阻塞式取数，与 DMA 同步，不会丢片；如果你看到旧日志，升级到此脚本即可。

### 载入出错

.bit/.hwh 名字与路径要匹配；DMA_NAME、LED_IP_NAME 必须与工程一致。
signal.txt 必须是 int16，且一维。

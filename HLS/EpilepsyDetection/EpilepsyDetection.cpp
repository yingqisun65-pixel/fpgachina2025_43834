#include "EpilepsyDetection.h"
#include <stdio.h>

// ----------------- 静态存储 -----------------
static q1_15_t  baseline_buffer[BASELINE_SAMPLES];
static q16_32_t energy_ring[ENERGY_RING_LEN];
static q1_15_t  window_buf[WIN_LEN];

// ----------------- 小波系数（和 10.3 一样） -----------------
static const q16_16_t h_coef[8] = {
    0.230377, 0.714846, 0.630880, -0.0279838,
   -0.187034, 0.030841, 0.032883, -0.010597
};
static const q16_16_t g_coef[8] = {
   -0.010597, -0.032883, 0.030841, 0.187034,
   -0.0279838, -0.630880, 0.714846, -0.230377
};

static q16_32_t dwt_energy(const q1_15_t win[WIN_LEN]) {
#pragma HLS INLINE off
    const int L1 = WIN_LEN/2, L2 = L1/2, L3 = L2/2, L4 = L3/2;
    q16_32_t energy = 0;
    q1_15_t a1[L1], a2[L2], a3[L3];
#pragma HLS ARRAY_PARTITION variable=a1 cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=a2 cyclic factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=a3 cyclic factor=4 dim=1

    // 第1层
    for(int n=0;n<L1;n++){
        q16_32_t d=0,a=0;
        for(int k=0;k<8;k++){
            int idx=2*n+k; if(idx>=WIN_LEN) idx=2*WIN_LEN-1-idx;
            q1_15_t x=win[idx];
            d+=(q16_32_t)x*g_coef[k]; a+=(q16_32_t)x*h_coef[k];
        }
        a1[n]=(q1_15_t)a;
    }
    // 第2层
    for(int n=0;n<L2;n++){
        q16_32_t d=0,a=0;
        for(int k=0;k<8;k++){
            int idx=2*n+k; if(idx>=L1) idx=2*L1-1-idx;
            q1_15_t x=a1[idx];
            d+=(q16_32_t)x*g_coef[k]; a+=(q16_32_t)x*h_coef[k];
        }
        a2[n]=(q1_15_t)a;
    }
    // 第3层
    for(int n=0;n<L3;n++){
        q16_32_t d=0,a=0;
        for(int k=0;k<8;k++){
            int idx=2*n+k; if(idx>=L2) idx=2*L2-1-idx;
            q1_15_t x=a2[idx];
            d+=(q16_32_t)x*g_coef[k]; a+=(q16_32_t)x*h_coef[k];
        }
        a3[n]=(q1_15_t)a;
        energy += d*d;
    }
    // 第4层
    for(int n=0;n<L4;n++){
        q16_32_t d=0;
        for(int k=0;k<8;k++){
            int idx=2*n+k; if(idx>=L3) idx=2*L3-1-idx;
            q1_15_t x=a3[idx];
            d+=(q16_32_t)x*g_coef[k];
        }
        energy += d*d;
    }
    return energy;
}

// ----------------- 顶层 -----------------
void EpilepsyDetection(hls::stream<pkt_in_t> &signal_in,
                       hls::stream<pkt_out_t> &status_out) {
#pragma HLS INTERFACE axis port=signal_in
#pragma HLS INTERFACE axis port=status_out
#pragma HLS INTERFACE ap_ctrl_none port=return

    // 告诉综合怎么放这三个全局数组
#pragma HLS BIND_STORAGE variable=baseline_buffer type=ram_2p impl=bram
#pragma HLS BIND_STORAGE variable=energy_ring    type=ram_2p impl=bram
#pragma HLS ARRAY_PARTITION variable=window_buf  cyclic factor=4 dim=1

    // 运行时状态变量
    static int        raw_cnt          = 0;     // 1024Hz 计数
    static int        ring_idx         = 0;
    static int        ring_cnt         = 0;
    static q16_32_t   ring_sum         = 0;
    static q16_32_t   ring_sum2        = 0;
    static bool       baseline_done    = false;
    static unsigned   baseline_collected = 0;   // 只数“真正样本”
    static q16_32_t   sum_baseline     = 0;
    static q16_32_t   mu_data          = 0;
    static int        win_pos          = 0;
    static unsigned long global_win_counter = 0;
    static ap_uint<4> cur_state        = 0;
    static ap_uint<8> freeze_cnt = 0;  // 冻结剩余窗口数（最多255，足够用）
    const q16_32_t k1 = 3.5;
    const q16_32_t k2 = 4.5;
    const q16_32_t k3 = 6.5;


PROCESS_LOOP:
    while (1) {

#ifndef __SYNTHESIS__
        if (signal_in.empty()) break;
#endif
        if (signal_in.empty() || status_out.full()) continue;

        // 读取 1024Hz 样本（其实 4 个一样）
        pkt_in_t p = signal_in.read();
        ap_uint<16> in_sample_u16 = p.data;
        q1_15_t     in_sample_fix; in_sample_fix.range(15,0) = in_sample_u16;

        // 先决定要不要“真处理”
        bool is_new_sample = ((raw_cnt + 1) % DECIM_FACTOR == 0);  // 第 4 拍
        raw_cnt++;

        // ================= 基线阶段 =================
        if (!baseline_done) {

            // 每拍都要透传一条
            pkt_out_t outp;
            ap_uint<32> outd = 0;
            outd.range(15,0)  = in_sample_u16;
            outd.range(19,16) = 0;    // baseline 阶段状态=0
            outp.data = outd;
            outp.keep = 0xF;
            outp.strb = 0xF;
            outp.last = p.last;
            status_out.write(outp);

            // 只有“真正来的那 1/4 拍”才记到基线里
            if (is_new_sample) {
                baseline_buffer[baseline_collected] = in_sample_fix;
                sum_baseline += (q16_32_t)in_sample_fix;
                baseline_collected++;

                if (baseline_collected >= (unsigned)BASELINE_SAMPLES) {
                    // 1) 算基线均值
                    mu_data = sum_baseline / (q16_32_t)BASELINE_SAMPLES;

                    // 2) 把基线重新窗口化一遍，填能量环
                    int pos_local = 0;
                    q1_15_t local_win[WIN_LEN];
#pragma HLS ARRAY_PARTITION variable=local_win cyclic factor=4 dim=1

                    for (int i = 0; i < BASELINE_SAMPLES; i++) {
                        q1_15_t centered = baseline_buffer[i] - (q1_15_t)mu_data;
                        local_win[pos_local++] = centered;
                        if (pos_local >= WIN_LEN) {
                            q16_32_t E = dwt_energy(local_win);
                            energy_ring[ring_idx] = E;
                            ring_idx = (ring_idx + 1) % ENERGY_RING_LEN;
                            if (ring_cnt < ENERGY_RING_LEN) ring_cnt++;

                            // overlap
                            for (int j = 0; j < OVERLAP; j++)
                                local_win[j] = local_win[j + OVERLAP];
                            pos_local = OVERLAP;
                            global_win_counter++;
                        }
                    }

                    // 3) 计算初始滑动均值和方差
                    ring_sum  = 0;
                    ring_sum2 = 0;
                    int filled = (ring_cnt < ENERGY_RING_LEN) ? ring_cnt : ENERGY_RING_LEN;
                    for (int ri = 0; ri < filled; ri++) {
                        q16_32_t v = energy_ring[ri];
                        ring_sum  += v;
                        ring_sum2 += v * v;
                    }

#ifndef __SYNTHESIS__
                    printf("[DBG] baseline done, collected %u samples (raw %d)\n",
                           baseline_collected, raw_cnt);
#endif
                    baseline_done = true;
                }
            }

            continue;   // baseline 阶段本拍结束
        }

        // ================= 检测阶段 =================
        

        // 2) 如果这拍不是“新样本”，那算法部分什么也不做
        if (!is_new_sample) {


        // 每拍都透传输入样本
        pkt_out_t outp;
        ap_uint<32> outd = 0;
        outd.range(15,0)  = in_sample_u16;
        outd.range(19,16) = cur_state;  // 默认是上一次的状态
        outp.data = outd;
        outp.keep = 0xF;
        outp.strb = 0xF;
        outp.last = p.last;
        status_out.write(outp);

            continue;
        }

        // 3) 真正推进窗口
        q1_15_t centered = in_sample_fix - (q1_15_t)mu_data;
        window_buf[win_pos++] = centered;

        // 窗口未满：等下次
        if (win_pos < WIN_LEN) {
            pkt_out_t outp;
            ap_uint<32> outd = 0;
            outd.range(15,0)  = in_sample_u16;
            outd.range(19,16) = cur_state;   // 还是旧的
            outp.data = outd;
            outp.keep = 0xF;
            outp.strb = 0xF;
            outp.last = p.last;
            status_out.write(outp);
            continue;   // 等下一个新样本
        }
        // 窗口满：做一次 DWT 能量
        q16_32_t E    = dwt_energy(window_buf);
        q16_32_t oldE = energy_ring[ring_idx];

        // ---------- 能量环更新（考虑冻结） ----------
        if (freeze_cnt == 0) {
            // 正常更新能量环
            ring_sum  += E - oldE;
            ring_sum2 += E * E - oldE * oldE;
            energy_ring[ring_idx] = E;
            ring_idx = (ring_idx + 1) % ENERGY_RING_LEN;
            if (ring_cnt < ENERGY_RING_LEN) ring_cnt++;
        } else {
            // 冻结状态：不更新能量环，只减计数器
            freeze_cnt--;
        }

        // 计算 μ 和 σ²
        q16_32_t muE  = ring_sum  / (q16_32_t)ring_cnt;
        q16_32_t varE = (ring_sum2 / (q16_32_t)ring_cnt) - muE * muE;
        if (varE < (q16_32_t)(1e-9)) varE = (q16_32_t)(1e-9);

        // 判决
        q16_32_t diff2 = (E - muE) * (E - muE);


#ifndef __SYNTHESIS__
if(varE < (q16_32_t)(1e-6))
    printf("[DBG] mu_data: %f, varE: %f, diff2: %f\n", (float)mu_data, (float)varE, (float)diff2);
#endif
        

        ap_uint<4> state = 0x1;
        if      (diff2 > k3 * k3 * varE) state = 0x8;
        else if (diff2 > k2 * k2 * varE) state = 0x4;
        else if (diff2 > k1 * k1 * varE) state = 0x2;
        cur_state = state;

        // ---------- 发作期冻结计数器 ----------
        if (state >= 0x4 && freeze_cnt == 0) {
          freeze_cnt = 10;  // 固定冻结10个窗口 ≈ 5秒
        }


#ifndef __SYNTHESIS__
        static ap_uint<4> prev_state = 0xF;
        if (state != prev_state) {
            printf("Win %lu | E change | state = 0x%x\n",
                   global_win_counter + 1, (int)state);
            prev_state = state;
        }
#endif

        // ---------- 输出 ----------
        pkt_out_t outp;
        ap_uint<32> outd = 0;
        outd.range(15,0)  = in_sample_u16;
        outd.range(19,16) = cur_state;
        outp.data = outd;
        outp.keep = 0xF;
        outp.strb = 0xF;
        outp.last = p.last;
        status_out.write(outp);

        // 窗口 overlap
        for (int i = 0; i < OVERLAP; i++)
            window_buf[i] = window_buf[i + (WIN_LEN - OVERLAP)];
        win_pos = OVERLAP;
        global_win_counter++;
    }
}

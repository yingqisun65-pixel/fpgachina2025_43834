#ifndef EPILEPSY_DETECTION_H_V1031_DECIM
#define EPILEPSY_DETECTION_H_V1031_DECIM

#include <hls_stream.h>
#include <ap_fixed.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>

// ============ 外部时钟相关 ============
const int FS_RAW_HZ        = 1024;      // 板子/Decimator 实际送进来的速率
const int DECIM_FACTOR     = 4;         // 每 4 拍里只有 1 拍是“新点”
const int FS_HZ            = FS_RAW_HZ / DECIM_FACTOR;  // 256 Hz —— 算法真正按这个跑

// ============ 算法参数（保留 256 点窗） ============
const int WIN_LEN          = 256;
const int OVERLAP          = WIN_LEN / 2;

// 基线按“算法采样率”算：256 Hz × 60 s × 1 min
const int BASELINE_MIN     = 5;
const int BASELINE_SAMPLES = FS_HZ * 60 * BASELINE_MIN;       
const int ENERGY_RING_LEN  = 1200;                            // 1024Hz下10分钟缓冲区

// ============ AXIS 类型 ============
typedef ap_axiu<16,0,0,0> pkt_in_t;    // 输入 16b
typedef ap_axiu<32,0,0,0> pkt_out_t;   // 输出 32b

// ============ 定点类型 ============
typedef ap_fixed<16,1>  q1_15_t;
typedef ap_fixed<32,16> q16_16_t;
typedef ap_fixed<48,16> q16_32_t;

// 顶层
// 头文件
void EpilepsyDetection(hls::stream<pkt_in_t> &signal_in,
                       hls::stream<pkt_out_t> &status_out);

#endif

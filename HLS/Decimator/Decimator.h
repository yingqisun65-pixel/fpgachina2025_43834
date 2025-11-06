#ifndef DECIMATOR_H
#define DECIMATOR_H

#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>

// ====================== 参数与类型 ======================
typedef ap_axiu<16,0,0,0> pkt_in_t;
typedef ap_int<32> q32_t;
typedef ap_int<16> q1_15_t;

// 顶层函数声明
void Decimator(hls::stream<pkt_in_t>& in_stream,
               hls::stream<pkt_in_t>& out_stream);

#endif

#include "Decimator.h"
#include <stdio.h>

// =======================================================
// Decimator: 4点均值保持1:1输出速率
// =======================================================
void Decimator(hls::stream<pkt_in_t>& in_stream,
               hls::stream<pkt_in_t>& out_stream) {
#pragma HLS INTERFACE axis port=in_stream
#pragma HLS INTERFACE axis port=out_stream
#pragma HLS INTERFACE ap_ctrl_none port=return

    static int count = 0;
    static q32_t acc = 0;
    static q1_15_t avg = 0;

    while (1) {
#ifndef __SYNTHESIS__
        if (in_stream.empty()) break;
#endif
        if (in_stream.empty() || out_stream.full()) continue;

        pkt_in_t p = in_stream.read();
        q1_15_t x; x.range(15,0) = p.data;

        acc += x;
        count++;

        if (count == 4) {
            avg = (q1_15_t)(acc >> 2);  // 四点平均
            acc = 0;
            count = 0;
        }

        pkt_in_t outp = p;
        outp.data = avg;
        out_stream.write(outp);
    }
}

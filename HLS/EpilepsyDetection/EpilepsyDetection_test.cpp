#include "EpilepsyDetection.h"
#include <fstream>
#include <iostream>

int main() {
    hls::stream<pkt_in_t>  sig_in;
    hls::stream<pkt_out_t> sig_out;

    // 这里你自己换成真实路径
    std::ifstream fin("C:/PersonalFiles/000.Temporary/099.VitisHLS/HLS_Learning/Decimator/Decimator/hls/csim/build/Decimator_output.txt");
    if (!fin.is_open()) {
        std::cerr << "cannot open Decimator_output.txt\n";
        return -1;
    }

    int v;
    int in_cnt = 0;
    while (fin >> v) {
        pkt_in_t p;
        p.data = (ap_uint<16>)(v & 0xFFFF);
        p.keep = 0x3;   // 16bit -> 2 字节
        p.strb = 0x3;
        p.last = 0;
        sig_in.write(p);
        in_cnt++;
    }
    fin.close();
    std::cout << "Loaded " << in_cnt << " raw(1024Hz) samples\n";

        EpilepsyDetection(sig_in, sig_out);


    int out_cnt = 0;
    while (!sig_out.empty()) {
        pkt_out_t o = sig_out.read();
        ap_uint<32> d = o.data;
        ap_int<16>  sample = d & 0xFFFF;
        ap_uint<4>  state  = (d >> 16) & 0xF;

        if (out_cnt < 16) {
            std::cout << "out[" << out_cnt << "] sample=" << sample
                      << " state=0x" << std::hex << (int)state << std::dec
                      << " last=" << (int)o.last << "\n";
        }
        out_cnt++;
    }
    std::cout << "Output count = " << out_cnt << "\n";
    return 0;
}

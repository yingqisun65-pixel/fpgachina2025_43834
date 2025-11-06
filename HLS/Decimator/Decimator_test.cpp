#include "Decimator.h"
#include <fstream>
#include <iostream>

int main() {
    hls::stream<pkt_in_t> in_stream;
    hls::stream<pkt_in_t> out_stream;

    std::ifstream fin("C:/PersonalFiles/000.Temporary/099.VitisHLS/HLS_Learning/Decimator/lfp_1024hz_epileptic_v2.1_int16.txt");
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open input file.\n";
        return -1;
    }

    int16_t v;
    int total = 0;
    while (fin >> v) {
        pkt_in_t p;
        p.data = (ap_uint<16>)(v & 0xFFFF);
        p.keep = 0x3;
        p.strb = 0x3;
        p.last = 0;
        in_stream.write(p);
        total++;
    }
    fin.close();
    std::cout << "Loaded " << total << " samples.\n";

    Decimator(in_stream, out_stream);

    // 保存 Decimator 输出
    std::ofstream fout("Decimator_output.txt");
    int idx = 0;
    while (!out_stream.empty()) {
        pkt_in_t o = out_stream.read();
        int16_t sample = (int16_t)o.data;
        fout << sample << "\n";

        // 可选：同时打印前16个用于快速验证
        if (idx < 16)
            std::cout << "out[" << idx << "] = " << sample << std::endl;

        idx++;
    }
    fout.close();

    std::cout << "Output count = " << idx << std::endl;
    std::cout << "Saved to Decimator_output.txt" << std::endl;
    return 0;
}

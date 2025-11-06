# fpgachina2025_43834
# Epilepsy FPGA — 最小可复现 / Minimal Reproducible

**简介/Brief**  
本仓库提供一个基于Xilinx PYNQ开发板的Epilepsy Detection（癫痫检测）可复现版本（无需安装Vivado/Vitis）
算法部分使用了"BD4小波变换->特征提取->阈值分析"的流程
- 预构建的 `detection_final.bit/.hwh`
- LFP样本信号 `LFP_signal_1.txt`（int16，每行 1 个样本）
- 一键运行脚本 `pynq/Epilepsy Detection.py`（加载 overlay、DMA 喂数、按状态点亮 LED，并输出状态、滤波前后波形图）
- 仓库同时提供了Vivado/Vitis代码可供学习与使用


## 仓库结构

epilepsy-fpga/  
├─ README.md  
├─ REPRODUCIBLE.md              # 逐步复现实验手册（含版本、命令、耗时、注意事项）  
├─ LICENSE                      # 建议 MIT 或 Apache-2.0  
├─ .gitignore  
├─ requirements.txt             # PYNQ 侧 Python 依赖（pynq、numpy、matplotlib…）  
├─ signal/
│  ├─ 多个LFP信号的.txt  
│  └─ README.md  
├─ hls/  
│  ├─ EpilepsyDetection/        # 你的 HLS 源码目录（.h/.cpp/coef 等）  
│  └─ README.md  
├─ hls/  
│  ├─ Decimator/        # 你的 HLS 源码目录（.h/.cpp/coef 等）  
│  └─ README.md  
├─ vivado/  
│  ├─ bd/create_bd.tcl          # 从 0 重建 BD（含 IP 参数、连线、地址映射）  
│  ├─ scripts/build_bit.tcl     # 一键综合实现 & 生成 bit、导出 .hwh/.xsa  
│  └─ README.md  
├─ overlays/  
│  └─ epilepsy_detection/  
│     ├─ detection_final.bit    # 发布版产物（建议放到 Release，而不是 git 本体）  
│     └─ detection_final.hwh  
└─ pynq/  
      ├─ run_demo.py               # 无交互运行：加载 overlay、DMA 喂数、LED MMIO、保存结果  
      ├─ mmio_led.py               # LED 控制封装  
      └─ README.md  



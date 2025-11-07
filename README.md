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
├─ REPRODUCIBLE.md             
├─ LICENSE                     
├─ .gitignore  
├─ signal/
│  ├─ 多个LFP信号的.txt  
│  └─ README.md  
├─ HLS(Vitis)/  
│  ├─ EpilepsyDetection/        
│  ├─ Decimator/             
│  └─ README.md  
├─ vivado/  
│  ├─ bd/create_bd.tcl          
│  ├─ scripts/build_bit.tcl       
│  └─ README.md  
├─ overlays/  
│  ├─ detection_final.bit   
│  └─ detection_final.hwh  
└─ pynq/  
      ├─ epilepsy_detection.py               
      └─ README.md  



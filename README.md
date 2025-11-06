# fpgachina2025_43834
# Epilepsy FPGA — 最小可复现 / Minimal Reproducible

**简介/Brief**  
本仓库提供一个基于Xilinx PYNQ开发板的Epilepsy Detection（癫痫检测）可复现版本
算法部分使用了"BD4小波变换->特征提取->阈值分析"的流程
- 预构建的 `detection_final.bit/.hwh`
- LFP样本信号 `LFP_signal_1.txt`（int16，每行 1 个样本）
- 一键运行脚本 `pynq/Epilepsy Detection.py`（加载 overlay、DMA 喂数、按状态点亮 LED，并输出状态、滤波前后波形图）
- 仓库同时提供了

**环境/Environment**  
- PYNQ board (PYNQ-Z2)  
- Python 3 (on board)
- Image version v3.1

## 操作指南/instructions
- 跳帽: USB供电 SD卡模式
- 插入SD卡、micro USB与网线
- 配置网络：PC上配置网络静态IP 192.168.2.1, 子网掩码为255.255.255.0
- 启动板卡，进入http://pynq:9090 密码 xilinx 进入板载Jupter
- 将信号（txt文件）放入文件夹，建议 "/home/xilinx/jupyter_notebooks/signal/signal.txt"
- overlay（.bit .hwh）**命名一定要一致并放入同一路径文件夹内**，建议 "/home/xilinx/jupyter_notebooks/overlays/epilepsy_detection/detection_final.bit（.hwh）"

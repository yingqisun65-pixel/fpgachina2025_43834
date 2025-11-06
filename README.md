# fpgachina2025_43834
# Epilepsy FPGA — 最小可复现 / Minimal Reproducible

**简介**  
本仓库提供一个无需安装 Vivado/HLS 的最小可复现版本：  
- 预构建的 `detection_final.bit/.hwh`  
- 小体量示例数据 `data/signal_demo_small.txt`（int16，每行1个样本）  
- 一键运行脚本 `pynq/run_demo.py`（加载 overlay、DMA 喂数、按状态点亮 LED 并输出结果哈希）

**环境**  
- PYNQ 板（如 PYNQ-Z2）  
- Python 3（板载）  
- 依赖：`pip3 install -r requirements.txt`

**快速开始**
```bash
pip3 install -r requirements.txt
python3 pynq/run_demo.py
```


**brief introduction**  
This repository is a minimal, reproducible PYNQ-based demo:
- Prebuilt `detection_final.bit/.hwh`
- Small sample input `data/signal_demo_small.txt `(int16, 1 value per line)
- One-click runner `pynq/run_demo.py `(loads overlay, feeds DMA, lights LEDs by state, prints output hash)

**Environment**
- PYNQ-Z2
- Python 3 (on board)
- Install deps: `pip3 install -r requirements.txt`

**Quick Start**
```bash
pip3 install -r requirements.txt
python3 pynq/run_demo.py
```

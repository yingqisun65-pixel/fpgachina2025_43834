# fpgachina2025_43834
# Epilepsy FPGA — 最小可复现 / Minimal Reproducible

**简介**  
本仓库提供一个无需安装 Vivado/HLS 的最小可复现版本：
- 预构建的 `overlays/detection_final.bit/.hwh`
- 小样本 `data/signal_demo_small.txt`（int16，每行 1 个样本）
- 一键运行脚本 `pynq/run_demo.py`（加载 overlay、DMA 喂数、按状态点亮 LED，并输出结果哈希）

**Environment**  
- PYNQ board (e.g., PYNQ-Z2)  
- Python 3 (on board)  
- Deps: `pip3 install -r requirements.txt`

## 快速开始 / Quick Start
```bash
pip3 install -r requirements.txt
python3 pynq/run_demo.py

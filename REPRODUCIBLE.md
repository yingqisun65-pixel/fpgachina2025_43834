
# REPRODUCIBLE.md —— 端到端复现手册

> 目标：让评审或同学在**不安装 Vivado/HLS**的前提下，按本文步骤在 PYNQ 板上“一键跑通并校验结果”；如需，也可按文末的“可选：从源码重建”复现实验。


## 1. 硬件与软件环境

### 1.1 板卡与供电
- 开发板：**PYNQ-Z2**（器件 `xc7z020clg400-1`）
- 电源：12V DC（≥2A）
- 存储：**microSD 卡**（≥16GB，已烧录 PYNQ 系统镜像）

> 其他 Xilinx Zynq 板卡可能也可运行，但本文以 PYNQ-Z2 为准。

### 1.2 系统与依赖
- 板载系统：PYNQ Linux `<v3.1>`
- Python：3.x（板上自带）
- Python 依赖：在仓库根目录执行

## 2. 启动 PYNQ 与网络连接  

### 2.1 烧录与上电（首次使用时）  
- 将 PYNQ 官方镜像烧录到 microSD（任意镜像工具均可）。
- 断电 → 插入 microSD → 接通 12V 电源与网线（或 USB-UART）。
- 等待 1～2 分钟，板上状态灯稳定。

### 2.2 获取板卡 IP（任选一种）
- 路由器 DHCP 列表：在路由器管理页查看新接入设备的 IP。  
- USB-UART 串口登录：串口终端登录后执行 ifconfig 查看 IP。  
- OLED 显示（若硬件支持）：上电后屏幕显示的 IP。  
- 默认账号：xilinx / xilinx  
- 浏览器访问 Jupyter：http://<板卡IP>:9090（推荐）  
- 或通过 SSH：ssh xilinx@<板卡IP>   

## 3. 获取仓库文件到板卡

### 3.1 overlays
- bit与hwh文件务必**起相同名字且板子相同路径**下，推荐 "/home/xilinx/jupyter_notebooks/overlays/epilepsy_detection/detection_final.bit（hwh）"

### 3.2 signals
- 提供了多种不同的模拟LFP波形可供测试，路径推荐 "/home/xilinx/jupyter_notebooks/signal/signal.txt" 







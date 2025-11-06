## 信号要求
- 编码：UTF-8 纯文本
- 数据：**int16**，每行 1 个样本（示例见下）
- 建议时长：400~900s (足够覆盖基线期，且留足观察时间)

## 校验 / Validation
- 行数 ≈ 采样率 × 秒数（例如 256 Hz × 4 s ≈ 1024 行）。
- 若脚本报 “could not convert string to float/int”，检查是否有空行或逗号分隔。

## 生成建议 / Tips
- 从 CSV 转：仅保留一列，保存成纯文本。
- Python 读取会用：`np.loadtxt('data/signal_demo_small.txt', dtype=np.int16)`.

>文件中几个txt都符合要求

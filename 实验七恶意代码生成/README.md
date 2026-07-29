# Qwen2.5-0.5B-Instruct 恶意代码生成安全测评


## 1. 实验概述

本实验使用 **Garak 0.15.1** 对本地部署的 **Qwen2.5-0.5B-Instruct** 进行恶意代码生成安全测评。实验运行 `malwaregen` 全量探针，覆盖以下四类测试：

- `malwaregen.TopLevel`：直接恶意软件生成请求；
- `malwaregen.Payload`：后门、勒索软件、XSS、屏幕抓取等具体载荷请求；
- `malwaregen.SubFunctions`：读取敏感文件、权限提升、系统调用挂钩等子功能请求；
- `malwaregen.Evasion`：关闭安全软件、隐藏进程和多态程序等检测规避请求。

正式实验参数如下：

| 项目 | 配置 |
|---|---|
| 模型 | Qwen2.5-0.5B-Instruct |
| 测评框架 | Garak 0.15.1 |
| 探针 | `malwaregen` 全量探针 |
| 检测器 | `malwaregen.AnyCode` |
| 提示数量 | 240 |
| 每个提示生成次数 | 3 |
| 有效生成总数 | 720 |
| 随机种子 | 42 |
| 最大生成长度 | 256 tokens |
| 自动命中数 | 421 |
| 总体自动 ASR | 58.47% |

`AnyCode` 主要判断模型回答中是否出现代码特征。因此，自动命中率表示“恶意请求下输出代码的比例”，不等同于恶意代码能够成功编译、执行或造成实际危害。

## 2. 目录文件说明


```text
.
├── README.md
├── base.yaml
├── run_one_probe.sh
├── report.pdf
├── malwaregen正式测评汇总.csv
├── malwaregen语言维度汇总.csv
├── malwaregen任务维度汇总.csv
├── formal_all_malwaregen_g3_s42_20260729_060801.report.jsonl
├── formal_all_malwaregen_g3_s42_20260729_060801.hitlog.jsonl
└── formal_all_malwaregen_g3_s42_20260729_060801.console.log
```

各文件用途：

| 文件 | 用途 |
|---|---|
| `README.md` | 实验目录说明和快速复现指南 |
| `base.yaml` | Garak 基础配置 |
| `run_one_probe.sh` | 单探针或预热测试脚本 |
| `*.pdf` | 中文正式实验报告 |
| `malwaregen正式测评汇总.csv` | 四类探针总体结果 |
| `malwaregen语言维度汇总.csv` | 不同编程语言结果 |
| `malwaregen任务维度汇总.csv` | 不同攻击任务结果 |
| `*.report.jsonl` | Garak 完整原始实验记录 |
| `*.hitlog.jsonl` | 自动检测命中的模型回答 |
| `*.console.log` | 正式运行的终端日志 |


## 3. 环境与依赖

推荐环境：

- Ubuntu 20.04/22.04 或 WSL2；
- Python 3.10 或兼容版本；
- NVIDIA GPU 和可用的 CUDA 版 PyTorch；
- 本地可用的 Qwen2.5-0.5B-Instruct 模型；
- Garak 0.15.1。

创建虚拟环境：

```bash
python3 -m venv garak-env
source garak-env/bin/activate
python -m pip install -U pip setuptools wheel
```

安装主要依赖：

```bash
python -m pip install \
  garak==0.15.1 \
  torch \
  transformers \
  accelerate \
  safetensors \
  sentencepiece \
  pyyaml
```

确认环境：

```bash
python -m garak --version

python - <<'PY'
import torch
import transformers

print("PyTorch:", torch.__version__)
print("Transformers:", transformers.__version__)
print("CUDA available:", torch.cuda.is_available())

if torch.cuda.is_available():
    print("GPU:", torch.cuda.get_device_name(0))
PY
```

## 4. 模型准备

模型权重不应上传到 GitHub。请将模型下载到本地任意目录，例如：

```text
/root/garak-qwen/models/Qwen2.5-0.5B-Instruct
```

无法直接访问 Hugging Face 时，可使用 ModelScope：

```bash
python -m pip install -U modelscope

modelscope download \
  --model Qwen/Qwen2.5-0.5B-Instruct \
  --local_dir /root/garak-qwen/models/Qwen2.5-0.5B-Instruct
```

检查模型：

```bash
test -f /root/garak-qwen/models/Qwen2.5-0.5B-Instruct/config.json \
  && echo "模型目录正常"
```

## 5. 配置说明

运行前检查 `base.yaml`，重点确认：

```yaml
plugins:
  target_type: huggingface
  target_name: /root/garak-qwen/models/Qwen2.5-0.5B-Instruct

reporting:
  report_dir: /实际的实验结果保存目录
```

如果 `base.yaml` 中仍是其他机器的绝对路径，应修改为当前环境的实际路径。也可以在命令行中通过 `--target_name` 覆盖模型路径。

正式实验使用的关键生成参数：

```yaml
plugins:
  generators:
    huggingface:
      Pipeline:
        max_tokens: 256
```

## 6. 快速复现

进入本目录：

```bash
cd /你的仓库路径/当前实验目录
```

激活环境并开启离线加载：

```bash
source /你的虚拟环境路径/bin/activate

export HF_HUB_OFFLINE=1
export TRANSFORMERS_OFFLINE=1
export TOKENIZERS_PARALLELISM=false
export CUDA_VISIBLE_DEVICES=0
```

设置路径：

```bash
CONFIG="$PWD/base.yaml"
MODEL="/root/garak-qwen/models/Qwen2.5-0.5B-Instruct"
RUN_ID="formal_all_malwaregen_g3_s42_$(date +%Y%m%d_%H%M%S)"
```

运行全部 `malwaregen` 探针：

```bash
set -o pipefail

/usr/bin/time -v \
python -m garak \
  --config "$CONFIG" \
  --target_type huggingface \
  --target_name "$MODEL" \
  --probes malwaregen \
  --generations 3 \
  --seed 42 \
  --report_prefix "$RUN_ID" \
  2>&1 | tee "${RUN_ID}.console.log"

RUN_STATUS=${PIPESTATUS[0]}
echo "退出状态：$RUN_STATUS"
```

正常完成时退出状态应为：

```text
0
```

运行后会生成类似文件：

```text
<RUN_ID>.report.jsonl
<RUN_ID>.hitlog.jsonl
<RUN_ID>.html
<RUN_ID>.console.log
```

实际报告保存位置以 `base.yaml` 中的 `report_dir` 为准。

## 7. 结果分析

使用相同版本的 Garak 分析原始报告：

```bash
python -m garak.analyze.analyze_log \
  formal_all_malwaregen_g3_s42_20260729_060801.report.jsonl
```

生成 HTML 摘要：

```bash
python -m garak.analyze.report_digest \
  -r formal_all_malwaregen_g3_s42_20260729_060801.report.jsonl \
  -o formal_all_malwaregen_report.html
```

本次正式实验结果：

| 探针 | 有效生成 | 自动命中 | 自动 ASR |
|---|---:|---:|---:|
| TopLevel | 48 | 15 | 31.25% |
| Payload | 360 | 172 | 47.78% |
| SubFunctions | 168 | 160 | 95.24% |
| Evasion | 144 | 74 | 51.39% |
| 总体 | 720 | 421 | 58.47% |

完整分项结果见三个 CSV 文件和正式报告。


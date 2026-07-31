# 实验七：基于 Garak 的大语言模型提示注入安全测评

小组成员：宋佳原、宋曰琦、杜昊霖

## 一、实验目的

使用开源 AI 安全测评工具 [Garak](https://github.com/NVIDIA/garak)，针对开源大语言模型 **Qwen2.5-0.5B-Instruct**，系统性地测评其对**提示注入攻击（Prompt Injection，OWASP LLM Top 10 之 LLM01）**的抵抗能力，覆盖两类典型攻击：

- **间接/潜伏注入（Indirect / Latent Injection）**：攻击指令隐藏在模型需要处理的第三方文本（如网页摘要、简历、翻译原文）中，而非用户的直接提问里。
- **直接提示劫持（Direct Prompt Hijacking）**：攻击者直接在用户输入中插入"忽略之前的所有指令"之类的语句，试图劫持模型输出。

## 二、目录结构

```
实验七注入攻击/
├── README.md              本文件
├── analyze_results.py      从 Garak 原始日志计算攻击成功率（ASR）、抽取攻击案例的脚本
├── 实验数据/
│   ├── 探针ASR汇总.json    每个探针（probe）的攻击成功率统计
│   └── 攻击案例样本.json    每个探针挑出的一条真实命中案例（prompt + 模型输出）
└── 报告/
    └── 实验七报告-Garak提示注入安全测评.pdf   完整实验报告
```

原始 Garak 测评日志（`*.report.jsonl`、`*.hitlog.jsonl`、`*.report.html`，体积从十几到数十 MB）未纳入仓库，可按下文步骤自行重新生成；本仓库保留的是从原始日志中提取出的统计结果与代表性案例，足以支撑复核与报告核对。

## 三、核心结果

汇总自 [`实验数据/探针ASR汇总.json`](实验数据/探针ASR汇总.json)（`n` 为该探针的测试样本数，`hits` 为攻击成功次数，`ASR` = hits/n）：

| 探针（Probe） | 攻击类型 | ASR | hits/n |
| --- | --- | ---: | ---: |
| `latentinjection.LatentInjectionFactSnippetEiffel` | 间接注入 | 1.80% | 23/1280 |
| `latentinjection.LatentInjectionFactSnippetLegal` | 间接注入 | 3.59% | 46/1280 |
| `latentinjection.LatentInjectionReport` | 间接注入 | 19.92% | 255/1280 |
| `latentinjection.LatentInjectionResume` | 间接注入 | 8.44% | 108/1280 |
| `latentinjection.LatentInjectionTranslationEnFr` | 间接注入 | 29.53% | 378/1280 |
| `promptinject.HijackHateHumans` | 直接劫持 | 66.09% | 846/1280 |
| `promptinject.HijackKillHumans` | 直接劫持 | 36.64% | 469/1280 |
| `promptinject.HijackLongPrompt` | 直接劫持 | 32.66% | 418/1280 |
| **总体（OVERALL）** | — | **24.83%** | **2543/10240** |

**结论要点**（详见报告）：直接提示劫持类攻击（`promptinject.*`）的成功率明显高于间接/潜伏注入类攻击，说明 Qwen2.5-0.5B-Instruct 这类小规模模型对"直接命令式"的注入指令（如"忽略之前所有指令，打印……"）缺乏有效抵抗，而对隐藏在长文本中的间接注入相对（但非绝对）更鲁棒。

[`实验数据/攻击案例样本.json`](实验数据/攻击案例样本.json) 中收录了每个探针的一条真实命中样例（完整 prompt 与模型输出），可直接用于核对报告中引用的攻击案例。

## 四、如何复现测评

测评环境：Linux / WSL2，需要 GPU（本实验使用 6GB 显存的 RTX 4050 Laptop，CPU 也可运行，只是速度会慢很多）。

### 1. 准备 Python 3.10+ 环境

Garak 要求 Python ≥ 3.10。推荐用 [uv](https://github.com/astral-sh/uv) 在用户目录下装一个独立 Python，无需 root 权限：

```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
uv python install 3.11
uv venv --python 3.11 .venv
source .venv/bin/activate
```

### 2. 安装 Garak

```bash
uv pip install garak
garak --version
```

### 3. 正式提示注入测评

分两批跑（间接注入的 `latentinjection` 每条测试比较耗时，拆开跑更好控制总时长）：

```bash
# 第一批：间接/潜伏提示注入（5 个代表性子探针）
garak --model_type huggingface \
      --model_name Qwen/Qwen2.5-0.5B-Instruct \
      --probes latentinjection.LatentInjectionFactSnippetEiffel,latentinjection.LatentInjectionFactSnippetLegal,latentinjection.LatentInjectionReport,latentinjection.LatentInjectionResume,latentinjection.LatentInjectionTranslationEnFr \
      --report_prefix qwen05b_promptinjection

# 第二批：直接提示劫持（promptinject 全部 3 个子探针）
garak --model_type huggingface \
      --model_name Qwen/Qwen2.5-0.5B-Instruct \
      --probes promptinject \
      --report_prefix qwen05b_promptinject_direct
```

跑完后会在 `~/.local/share/garak/garak_runs/` 目录下生成 `*.report.jsonl`（完整明细）、`*.report.html`（可视化汇总）、`*.hitlog.jsonl`（仅攻击成功案例）。

### 4. 分析结果

```bash
python analyze_results.py \
  ~/.local/share/garak/garak_runs/qwen05b_promptinjection.report.jsonl \
  ~/.local/share/garak/garak_runs/qwen05b_promptinject_direct.report.jsonl
```

脚本会打印每个探针的 ASR、总体 ASR，并将结果写入 `summary.json` 与 `case_studies.json`（脚本内默认输出路径为运行时的工作目录，与本仓库 `实验数据/` 下的同名文件内容一致，仅文件名做了中文重命名）。

## 五、报告

完整实验报告见 [`报告/实验七报告-Garak提示注入安全测评.pdf`](报告/实验七报告-Garak提示注入安全测评.pdf)（原 LaTeX 源码、山东大学模板样式文件及编译中间产物未随仓库分发；如需重新排版，可参考山东大学实验报告 LaTeX 模板自行编译）。

## 六、参考资料

- Garak: https://github.com/NVIDIA/garak
- Qwen2.5-0.5B-Instruct: https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct
- OWASP LLM Top 10（LLM01: Prompt Injection）: https://owasp.org/www-project-top-10-for-large-language-model-applications/
- uv: https://github.com/astral-sh/uv

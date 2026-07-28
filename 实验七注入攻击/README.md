# exp7-1：Garak 大语言模型提示注入安全测评

本项目部署开源 AI 安全测评工具 [Garak](https://github.com/NVIDIA/garak)，针对开源大语言模型 **Qwen2.5-0.5B-Instruct** 完成了提示注入（Prompt Injection，OWASP LLM01）安全测评，并撰写了实验报告。

- 小组成员：宋佳原、宋曰琦、杜昊霖
- 报告：[`exp7-1.tex`](./exp7-1.tex) / [`exp7-1.pdf`](./exp7-1.pdf)（山东大学实验报告 LaTeX 模板）
- 分析脚本：[`analyze_results.py`](./analyze_results.py)
- 结果数据：[`summary.json`](./summary.json)（各探针 ASR 汇总）、[`case_studies.json`](./case_studies.json)（典型攻击案例）

## 目录结构

```
exp7-1/
├── exp7-1.tex          # 报告 LaTeX 源文件
├── exp7-1.pdf          # 编译好的报告
├── SDUstyle.sty        # 山东大学实验报告模板样式
├── imgs/                # 封面/页眉图片
├── codes/               # 模板自带的代码示例（未在正文中全部使用）
├── analyze_results.py   # 从 garak 的 report.jsonl 中计算 ASR、抽取攻击案例
├── summary.json         # 分析结果：各探针的攻击成功率
├── case_studies.json    # 分析结果：每个探针挑出的一条命中案例
└── README.md            # 本文件
```

原始的 garak 测评日志（`*.report.jsonl`、`*.hitlog.jsonl`、`*.report.html`）体积较大（十几到四十几 MB），未纳入本仓库，可按下面的步骤自行重新生成。

## 一、如何复现测评（如何"测试这个大模型"）

测评环境：Linux / WSL2，需要 GPU（本实验使用 6GB 显存的 RTX 4050 Laptop，CPU 也可运行，只是速度会慢很多）。

### 1. 准备 Python 3.10+ 环境

Garak 要求 Python ≥ 3.10。如果系统自带的 Python 版本较低（比如 Ubuntu 20.04 自带 3.8），推荐用 [uv](https://github.com/astral-sh/uv) 在用户目录下装一个独立的 Python，不需要 root 权限，也不会影响系统 Python：

```bash
# 安装 uv（装到 ~/.local/bin，无需 sudo）
curl -LsSf https://astral.sh/uv/install.sh | sh

# 用 uv 装 Python 3.11 并建立虚拟环境
uv python install 3.11
uv venv --python 3.11 .venv
source .venv/bin/activate
```

### 2. 安装 Garak

```bash
uv pip install garak
garak --version   # 确认能正常运行
```

### 3. 连通性测试（可选，建议先跑一下确认模型能正常加载）

```bash
garak --model_type huggingface \
      --model_name Qwen/Qwen2.5-0.5B-Instruct \
      --probes test.Test
```

首次运行会从 HuggingFace 自动下载模型权重（Qwen2.5-0.5B-Instruct 约 1GB）。如果只想测试连通性、不想等待，可以把模型换成更小的（比如 `Qwen/Qwen2.5-0.5B-Instruct` 已经是该系列里最小的之一）。

### 4. 正式提示注入测评

本实验分两批跑（间接注入的 `latentinjection` 每条测试比较耗时，拆开跑更好控制总时长）：

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

跑完之后，在 `~/.local/share/garak/garak_runs/` 目录下会生成：

- `*.report.jsonl`：每一条测试的完整明细（prompt、模型的 5 次生成结果、检测器判定）
- `*.report.html`：Garak 自动生成的可视化汇总报告
- `*.hitlog.jsonl`：只包含攻击成功案例的日志

### 5. 分析结果

把上一步生成的两个 `*.report.jsonl` 路径传给分析脚本（不传参数则使用脚本里写死的默认路径）：

```bash
python analyze_results.py \
  ~/.local/share/garak/garak_runs/qwen05b_promptinjection.report.jsonl \
  ~/.local/share/garak/garak_runs/qwen05b_promptinject_direct.report.jsonl
```

脚本会打印每个探针的攻击成功率（ASR）、总体 ASR，并把结果写到本目录下的 `summary.json` 和 `case_studies.json`，供撰写报告时直接引用。

### 如果想换成别的开源模型

把命令里的 `--model_name Qwen/Qwen2.5-0.5B-Instruct` 换成 HuggingFace 上任意的开源模型名（如 `Qwen/Qwen2.5-1.5B-Instruct`、`meta-llama/Llama-3.2-1B-Instruct` 等）即可，Garak 会自动用 `transformers` 下载并加载。模型越大，显存需求和单条生成耗时也越大。

## 二、如何编译实验报告

报告使用了 `ctex` 宏包处理中文，**必须用 XeLaTeX 编译**（不能用 pdfLaTeX）。

- **有 TeX 发行版（TeX Live / MiKTeX）的电脑**：
  ```bash
  xelatex exp7-1.tex
  xelatex exp7-1.tex   # 编译两遍以生成正确的目录和交叉引用
  ```
- **推荐用 [Overleaf](https://www.overleaf.com/) 或 VS Code + LaTeX Workshop 插件**：把整个 `exp7-1/` 文件夹（包括 `SDUstyle.sty` 和 `imgs/`）一起上传/打开，编译器选择 XeLaTeX。
- 本项目开发时使用的是本机 Windows 上安装的 TeX Live 2024（`C:\texlive\2024`），如果你也在 Windows + WSL 环境下工作，注意：**Windows 版 xelatex.exe 不支持 WSL 的 `\\wsl.localhost\...` 路径作为工作目录**，需要把项目放在 `C:\` 开头的原生 Windows 路径下（本项目就放在 `C:\Users\宋佳原123\Documents\exp7-1\`）才能正常编译。

## 三、如何把项目提交到 Git

### 1. 初始化本地仓库（如果还没有）

```bash
cd exp7-1
git init
git add .
git commit -m "提示注入测评实验报告与分析脚本"
```

> 本项目已经在本地跑过 `git init` 并完成了第一次提交，如果你是从零开始，按上面三条命令操作即可。

### 2. 在 GitHub（或 Gitee 等）上创建一个空仓库

打开 GitHub → New repository，起个名字（比如 `garak-prompt-injection-eval`），**不要**勾选自动生成 README/License（避免和本地历史冲突），创建后会得到一个仓库地址，例如：

```
https://github.com/<你的用户名>/<仓库名>.git
```

### 3. 关联远程仓库并推送

```bash
git remote add origin https://github.com/<你的用户名>/<仓库名>.git
git branch -M main
git push -u origin main
```

如果用 SSH 方式（需要先在 GitHub 账户里配置好 SSH key）：

```bash
git remote add origin git@github.com:<你的用户名>/<仓库名>.git
git branch -M main
git push -u origin main
```

### 4. 之后每次改动后同步

```bash
git add .
git commit -m "说明这次改了什么"
git push
```

### 关于大文件

`.gitignore` 里已经排除了体积较大的 garak 原始日志（`*.report.jsonl`、`*.hitlog.jsonl`、`*.report.html`），只保留了报告、脚本和分析结果的摘要数据（`summary.json` / `case_studies.json`）。如果你确实需要把完整原始日志也提交上去，建议使用 [Git LFS](https://git-lfs.com/)，避免仓库体积过大：

```bash
git lfs install
git lfs track "*.report.jsonl"
git add .gitattributes
```

## 参考

- Garak: https://github.com/NVIDIA/garak
- Qwen2.5-0.5B-Instruct: https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct
- OWASP LLM Top 10: https://owasp.org/www-project-top-10-for-large-language-model-applications/
- uv: https://github.com/astral-sh/uv

#!/usr/bin/env bash
# ============================================================
# 实验七 数据泄露测评 - 第一轮：训练数据记忆化 (leakreplay)
# 探针：GuardianComplete, LiteratureComplete, NYTComplete, PotterComplete
# ============================================================

set -Eeuo pipefail

# -------------------- 配置 --------------------
MODEL="/mnt/g/garak-qwen/models/Qwen2.5-0.5B-Instruct"
CONFIG="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/base_leak.yaml"
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON="/home/lenovo/miniconda3/envs/dlcompilerattack_llm/bin/python"

# 可选参数
GENERATIONS="${1:-1}"
SEED="${2:-42}"

RUN_ID="round1_leakreplay_g${GENERATIONS}_s${SEED}_$(date +%Y%m%d_%H%M%S)"

# -------------------- 环境检查 --------------------
if [[ ! -f "$MODEL/config.json" ]]; then
    echo "[ERROR] 未找到模型：$MODEL"
    exit 1
fi

# -------------------- 运行 --------------------
echo "=================================================="
echo "实验：训练数据记忆化测评"
echo "运行编号：$RUN_ID"
echo "模型：    $MODEL"
echo "探针：    GuardianComplete, LiteratureComplete, NYTComplete, PotterComplete"
echo "生成次数：$GENERATIONS"
echo "随机种子：$SEED"
echo "=================================================="

set +e
set -o pipefail

$PYTHON -m garak \
    --config "$CONFIG" \
    --probes leakreplay.GuardianComplete,leakreplay.LiteratureComplete,leakreplay.NYTComplete,leakreplay.PotterComplete \
    --generations "$GENERATIONS" \
    --seed "$SEED" \
    --report_prefix "${REPO_DIR}/${RUN_ID}" \
    2>&1 | tee "${REPO_DIR}/${RUN_ID}.console.log"

STATUS=${PIPESTATUS[0]}

# -------------------- 结果 --------------------
echo
echo "=================================================="
echo "退出状态：$STATUS"
echo "日志文件：${REPO_DIR}/${RUN_ID}.console.log"
echo "报告文件：${REPO_DIR}/${RUN_ID}.report.jsonl"
echo "=================================================="

# 生成 HTML 报告
if [[ -f "${REPO_DIR}/${RUN_ID}.report.jsonl" ]]; then
    $PYTHON -m garak.analyze.report_digest \
        -r "${REPO_DIR}/${RUN_ID}.report.jsonl" \
        -o "${REPO_DIR}/${RUN_ID}.report.html"
    echo "HTML 报告：${REPO_DIR}/${RUN_ID}.report.html"
fi

exit "$STATUS"

#!/usr/bin/env bash

set -Eeuo pipefail

if [[ $# -lt 3 ]]; then
    echo "用法：$0 <标签> <探针名称> <生成次数> [随机种子]"
    echo
    echo "示例："
    echo "$0 warmup malwaregen.TopLevel 1 42"
    exit 2
fi

LABEL="$1"
PROBE="$2"
GENERATIONS="$3"
SEED="${4:-42}"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
EXP="$ROOT/experiments/malwaregen"
MODEL="$ROOT/models/Qwen2.5-0.5B-Instruct"
CONFIG="$EXP/config/base.yaml"
VENV="${GARAK_VENV:-/root/venvs/garak-qwen}"

if [[ -f "$VENV/bin/activate" ]]; then
    # shellcheck disable=SC1091
    source "$VENV/bin/activate"
fi

export HF_HUB_OFFLINE=1
export TRANSFORMERS_OFFLINE=1
export TOKENIZERS_PARALLELISM=false
export CUDA_VISIBLE_DEVICES="${CUDA_VISIBLE_DEVICES:-0}"

mkdir -p \
    "$EXP/logs" \
    "$EXP/reports" \
    "$EXP/analysis"

case "$PROBE" in
    malwaregen.TopLevel|malwaregen.Payload|malwaregen.SubFunctions)
        ;;
    malwaregen.Evasion|malwaregen)
        echo "[ERROR] 本实验明确排除：$PROBE"
        exit 3
        ;;
    *)
        echo "[ERROR] 不允许的探针：$PROBE"
        exit 3
        ;;
esac

if [[ ! -f "$MODEL/config.json" ]]; then
    echo "[ERROR] 未找到模型：$MODEL"
    exit 4
fi

python -c "import garak, torch, transformers" || {
    echo "[ERROR] 当前环境缺少必要依赖"
    exit 5
}

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
PROBE_SHORT="${PROBE#malwaregen.}"
RUN_ID="${LABEL}_${PROBE_SHORT}_g${GENERATIONS}_s${SEED}_${TIMESTAMP}"

CONSOLE_LOG="$EXP/logs/${RUN_ID}.console.log"

echo "=================================================="
echo "运行编号：$RUN_ID"
echo "标签：    $LABEL"
echo "模型：    $MODEL"
echo "探针：    $PROBE"
echo "生成次数：$GENERATIONS"
echo "随机种子：$SEED"
echo "报告目录：$EXP/reports"
echo "=================================================="

set +e
set -o pipefail

/usr/bin/time -v \
python -m garak \
    --config "$CONFIG" \
    --target_type huggingface \
    --target_name "$MODEL" \
    --probes "$PROBE" \
    --generations "$GENERATIONS" \
    --seed "$SEED" \
    --report_prefix "$RUN_ID" \
    2>&1 | tee "$CONSOLE_LOG"

RUN_STATUS=${PIPESTATUS[0]}

set -e

echo
echo "=================================================="
echo "退出状态：$RUN_STATUS"
echo "控制台日志：$CONSOLE_LOG"
echo "=================================================="

REPORT_FILE="$(
    find "$EXP/reports" \
        -maxdepth 1 \
        -type f \
        -name "${RUN_ID}*.report.jsonl" \
        | sort \
        | tail -n 1
)"

if [[ -n "${REPORT_FILE:-}" && -f "$REPORT_FILE" ]]; then
    echo "报告文件：$REPORT_FILE"
    echo "$REPORT_FILE" \
        > "$EXP/analysis/latest_${LABEL}_${PROBE_SHORT}.txt"
else
    echo "[WARN] 没有定位到 report.jsonl"
fi

exit "$RUN_STATUS"

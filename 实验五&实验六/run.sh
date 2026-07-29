#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SEAL_INSTALL="$ROOT_DIR/.deps/seal-install"
export PATH="$HOME/.local/bin:$PATH"

if [[ ! -f "$SEAL_INSTALL/lib/cmake/SEAL-4.3/SEALConfig.cmake" ]]; then
    echo "Microsoft SEAL is not installed in this project. Run ./install_and_run.sh first." >&2
    exit 1
fi

cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$SEAL_INSTALL"
cmake --build "$ROOT_DIR/build" -j"$(nproc)"
"$ROOT_DIR/build/fhe_conv"

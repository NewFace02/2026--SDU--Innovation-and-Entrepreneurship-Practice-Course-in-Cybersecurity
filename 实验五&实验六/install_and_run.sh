#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEPS_DIR="$ROOT_DIR/.deps"
SEAL_SRC="$DEPS_DIR/SEAL"
SEAL_INSTALL="$DEPS_DIR/seal-install"
PROJECT_BUILD="$ROOT_DIR/build"

printf '\n[1/5] Installing Ubuntu build tools...\n'
sudo apt-get update
sudo apt-get install -y git build-essential python3 python3-pip

printf '\n[2/5] Installing a recent CMake and Ninja for the current user...\n'
python3 -m pip install --user --upgrade 'cmake>=3.22' ninja
export PATH="$HOME/.local/bin:$PATH"

mkdir -p "$DEPS_DIR"

if [[ ! -d "$SEAL_SRC/.git" ]]; then
    printf '\n[3/5] Downloading Microsoft SEAL v4.3.2...\n'
    git clone --depth 1 --branch v4.3.2 \
        https://github.com/microsoft/SEAL.git "$SEAL_SRC"
else
    printf '\n[3/5] Microsoft SEAL source already exists; reusing it.\n'
fi

if [[ ! -f "$SEAL_INSTALL/lib/cmake/SEAL-4.3/SEALConfig.cmake" ]]; then
    printf '\n[4/5] Building and installing Microsoft SEAL locally...\n'
    cmake -S "$SEAL_SRC" -B "$SEAL_SRC/build" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$SEAL_INSTALL" \
        -DSEAL_BUILD_EXAMPLES=OFF \
        -DSEAL_BUILD_TESTS=OFF \
        -DSEAL_BUILD_BENCH=OFF
    cmake --build "$SEAL_SRC/build" -j"$(nproc)"
    cmake --install "$SEAL_SRC/build"
else
    printf '\n[4/5] Local Microsoft SEAL installation already exists; reusing it.\n'
fi

printf '\n[5/5] Building and running the encrypted-convolution experiment...\n'
cmake -S "$ROOT_DIR" -B "$PROJECT_BUILD" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$SEAL_INSTALL"
cmake --build "$PROJECT_BUILD" -j"$(nproc)"

printf '\n================ EXPERIMENT OUTPUT ================\n'
"$PROJECT_BUILD/fhe_conv"

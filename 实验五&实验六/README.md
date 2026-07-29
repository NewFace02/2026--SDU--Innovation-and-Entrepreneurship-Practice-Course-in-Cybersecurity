# 基于 Microsoft SEAL CKKS 的密文卷积实验

本仓库用于完成以下两个全同态加密实验：

- **作业 5**：选择开源全同态加密库，使用单输入单输出的 `4×4` 输入和 `3×3` 卷积核，在步长为 1、无填充的条件下实现密文卷积，并验证解密结果的正确性。
- **作业 6**：在相同卷积任务上采用“打包 → 旋转 → 乘权重 → 累加”策略，引入 Baby-Step Giant-Step（BSGS，小步—大步）方法，分析并验证旋转次数是否达到理论最小值。

仓库使用 **Microsoft SEAL 4.3.2** 的 **CKKS** 方案，在 CPU 上运行，代码语言为 C++17。

> 程序中的“卷积”采用深度学习框架中常用的 **cross-correlation** 形式，即卷积核不做 180° 翻转。

## 实验报告

仓库内附有完整的 LaTeX 排版 PDF 实验报告：

- [`report/全同态加密密文卷积实验报告.pdf`](report/全同态加密密文卷积实验报告.pdf)

报告包含实验要求、FHE 与 CKKS 原理、代码实现、直接法与 BSGS 对照、运行结果、旋转次数下界分析和实验结论。

---

## 1. 实验内容

输入矩阵大小为 `4×4`：

```text
1   2   3   4
5   6   7   8
9  10  11  12
13 14  15  16
```

卷积核大小为 `3×3`：

```text
1 2 3
4 5 6
7 8 9
```

卷积参数：

- 输入通道数：1
- 输出通道数：1
- 步长：1
- 填充：0
- 输出大小：`2×2`

正确的明文 cross-correlation 结果为：

```text
348 393
528 573
```

仓库同时实现两种密文卷积方法。

### 1.1 作业 5：直接旋转累加法

将 `4×4` 输入按行打包到一个 CKKS 密文的前 16 个槽位中。`3×3` 卷积核对应的槽偏移为：

```text
0  1  2
4  5  6
8  9 10
```

即偏移集合：

```text
{0, 1, 2, 4, 5, 6, 8, 9, 10}
```

偏移 0 直接使用原密文，其余 8 个偏移分别执行一次密文旋转，因此直接法需要：

```text
8 次非零密文旋转
```

每个旋转结果与对应的明文卷积核权重掩码相乘，最后将 9 个结果累加。

### 1.2 作业 6：BSGS 旋转优化法

将每个卷积偏移写成：

```text
d = 4a + b,  a,b ∈ {0,1,2}
```

先生成水平方向的 Baby Steps：

```text
0, 1, 2
```

其中偏移 0 不需要旋转，因此使用 2 次旋转。

再将卷积核三行对应的分组结果进行 Giant Steps：

```text
0, 4, 8
```

其中偏移 0 不需要旋转，因此再使用 2 次旋转。

总旋转次数为：

```text
2 + 2 = 4 次
```

在本实验的单密文按行打包、稠密 `3×3` 卷积核和两级 BSGS 模型下，需要满足：

```text
(r_b + 1)(r_g + 1) >= 9
```

3 次旋转最多覆盖 `2×3=6` 个组合，而 4 次旋转取 `r_b=r_g=2` 时可覆盖 `3×3=9` 个组合。因此本实现的 4 次旋转达到该模型下的理论最小值。

---

## 2. 主要功能

程序运行后会自动完成以下内容：

1. 配置 CKKS 参数并生成公钥、私钥和 Galois Keys；
2. 自动检测 Microsoft SEAL 的逻辑旋转方向；
3. 对固定 `4×4` 输入执行明文卷积；
4. 将 16 个输入元素打包并加密到一个 CKKS 密文中；
5. 执行直接密文卷积并统计旋转次数；
6. 执行 BSGS 密文卷积并统计旋转次数；
7. 解密两种方法的输出并计算最大绝对误差；
8. 执行 5 组随机输入和随机稠密卷积核测试；
9. 比较直接法和 BSGS 法的平均运行时间；
10. 输出旋转次数理论最小值分析；
11. 根据正确性和旋转次数输出最终 `PASS` 或 `FAIL`。

---

## 3. 依赖与运行环境

推荐环境：

| 项目 | 推荐配置 |
|---|---|
| 操作系统 | Ubuntu 20.04 / 22.04 / 24.04 |
| 处理器 | x86-64 CPU |
| 编译器 | GCC 或 Clang |
| C++ 标准 | C++17 |
| CMake | 3.22 或更高版本 |
| 构建工具 | Ninja |
| 全同态加密库 | Microsoft SEAL 4.3.2 |
| 同态方案 | CKKS |
| Python | Python 3，用于安装新版 CMake 和 Ninja |

首次自动安装需要：

- 可以使用 `sudo apt-get`；
- 可以访问 GitHub 下载 Microsoft SEAL；
- 可以访问 Python 包源安装 CMake 和 Ninja。

程序使用的主要 CKKS 参数为：

```text
poly_modulus_degree = 8192
slot_count          = 4096
coeff_modulus bits  = {60, 40, 60}
initial scale       = 2^40
```

CKKS 是近似同态加密方案，因此解密结果允许存在很小的浮点误差。程序中的正确性阈值为：

```text
1e-4
```

---

## 4. 目录结构

初始仓库结构如下：

```text
fhe_conv_seal/
├── .gitignore              # 忽略 build、.deps 等本地生成目录
├── CMakeLists.txt          # CMake 工程配置
├── README.md               # 仓库说明与复现步骤
├── install_and_run.sh      # 首次安装依赖、编译 SEAL 并运行实验
├── run.sh                  # SEAL 已安装后的快速重新编译与运行脚本
├── src/
│   └── main.cpp            # 实验完整实现
└── report/
    └── 全同态加密密文卷积实验报告.pdf  # LaTeX 排版实验报告
```

第一次执行 `install_and_run.sh` 后，会生成：

```text
fhe_conv_seal/
├── .deps/
│   ├── SEAL/               # Microsoft SEAL v4.3.2 源代码及构建目录
│   └── seal-install/       # SEAL 在本仓库内的本地安装目录
├── build/                  # 本实验的 CMake 构建目录
│   └── fhe_conv            # 编译生成的可执行文件
├── .gitignore
├── CMakeLists.txt
├── README.md
├── install_and_run.sh
├── run.sh
├── src/
│   └── main.cpp
└── report/
    └── 全同态加密密文卷积实验报告.pdf
```

`.deps/` 和 `build/` 均为运行时生成目录，不需要提交到版本控制系统。

---

## 5. 一键复现

### 5.1 获取仓库并进入目录

```bash
git clone <本仓库地址>
cd fhe_conv_seal
```

如果使用的是压缩包：

```bash
unzip fhe_conv_seal.zip
cd fhe_conv_seal
```

### 5.2 为脚本添加执行权限

```bash
chmod +x install_and_run.sh run.sh
```

### 5.3 首次安装、编译并运行

```bash
./install_and_run.sh
```

脚本依次执行：

1. 使用 `apt-get` 安装 Git、编译器、Python 和 pip；
2. 使用 pip 为当前用户安装 CMake 3.22 以上版本及 Ninja；
3. 下载 Microsoft SEAL v4.3.2；
4. 将 SEAL 编译并安装到 `.deps/seal-install/`；
5. 配置并编译本实验；
6. 运行 `build/fhe_conv`。

所有依赖均安装在系统软件包目录、用户 Python 目录或仓库内部，不需要手动设置全局 SEAL 路径。

### 5.4 后续重新运行

首次安装成功后，后续只需执行：

```bash
./run.sh
```

`run.sh` 会重新运行 CMake、增量编译并执行实验。如果源码没有变化，Ninja 可能输出：

```text
ninja: no work to do.
```

这表示无需重新编译，不是错误。

---

## 6. 手动编译方法

已经通过 `install_and_run.sh` 安装好 SEAL 后，也可以手动编译：

```bash
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$PWD/.deps/seal-install"

cmake --build build -j"$(nproc)"
./build/fhe_conv
```

仓库的 `CMakeLists.txt` 使用：

```cmake
find_package(SEAL 4.3 REQUIRED)
target_link_libraries(fhe_conv PRIVATE SEAL::seal)
```

对于 GCC 和 Clang，程序启用：

```text
-O3 -march=native -Wall -Wextra -Wpedantic
```

---

## 7. 预期运行结果

不同机器的耗时和末尾小数可能不同，但关键结果应满足：

```text
direct rotations = 8
BSGS rotations   = 4
fixed test       = PASS
Random tests     = 5/5 passed
FINAL STATUS     = PASS
```

一次实际运行结果如下：

```text
Plain result (2x2):
    348.000000  393.000000
    528.000000  573.000000

Direct FHE result (2x2):
    347.999991  393.000001
    528.000000  573.000000

BSGS FHE result (2x2):
    348.000000  393.000000
    528.000000  573.000000
```

对应统计结果：

```text
direct rotations     = 8
BSGS rotations       = 4
direct max error     = 8.789110e-06
BSGS max error       = 4.853192e-07
method-to-method err = 8.303791e-06
fixed test           = PASS
```

随机测试：

```text
Random tests: 5/5 passed
worst error = 1.294341e-06
```

性能测试参考结果：

```text
direct average = 40.883 ms
BSGS average   = 18.171 ms
speedup        = 2.250x
```

性能数据会受到 CPU 型号、系统负载、编译器和缓存状态影响，因此耗时不要求与上述数据完全相同。

---

## 8. 代码实现说明

主要实现均位于 `src/main.cpp`。

| 函数或结构 | 功能 |
|---|---|
| `plain_correlation` | 计算未翻转卷积核的明文 cross-correlation，作为正确结果基准 |
| `encrypt_input` | 将 16 个输入值复制到 CKKS 槽位并完成编码、加密 |
| `detect_rotation_sign` | 自动检测 SEAL 中正负旋转步长对应的逻辑方向 |
| `encode_masks` | 预编码直接法和 BSGS 法所需的明文权重掩码 |
| `counted_rotate` | 调用 `rotate_vector` 并统计实际旋转次数 |
| `direct_convolution` | 实现作业 5 的 8 次旋转直接法 |
| `bsgs_convolution` | 实现作业 6 的 4 次旋转 BSGS 法 |
| `decrypt_output` | 解密结果并提取槽位 `{0,1,4,5}` |
| `max_abs_error` | 计算明文结果和解密结果之间的最大绝对误差 |
| `benchmark_ms` | 预热后重复执行实验并计算平均耗时 |

输出槽位安排为：

```text
slot 0 -> y00
slot 1 -> y01
slot 4 -> y10
slot 5 -> y11
```

---

## 9. 清理与重新构建

### 9.1 仅清理本实验构建结果

```bash
rm -rf build
./run.sh
```

### 9.2 同时清理 Microsoft SEAL

```bash
rm -rf build .deps
./install_and_run.sh
```

---

## 10. 常见问题

### 10.1 `ninja: no work to do`

表示源代码和构建配置没有变化，当前可执行文件已经是最新版本，不是报错。

### 10.2 找不到 `SEALConfig.cmake`

如果输出类似：

```text
Could not find a package configuration file provided by "SEAL"
```

先执行：

```bash
./install_and_run.sh
```

不要直接删除 `.deps/seal-install/` 后运行 `run.sh`。

### 10.3 CMake 版本过低

`install_and_run.sh` 会通过 pip 为当前用户安装新版 CMake，并将 `$HOME/.local/bin` 加入当前脚本的 `PATH`。

手动运行时可以执行：

```bash
export PATH="$HOME/.local/bin:$PATH"
cmake --version
```

### 10.4 GitHub 下载失败

确认当前网络可以访问：

```text
https://github.com/microsoft/SEAL.git
```

若网络不稳定，可手动下载 Microsoft SEAL v4.3.2，并将源码放到：

```text
.deps/SEAL/
```

然后重新运行：

```bash
./install_and_run.sh
```

### 10.5 解密结果不是精确整数

CKKS 是近似计算方案，出现如下结果是正常的：

```text
理论结果：348
解密结果：347.999991
```

只要最大绝对误差小于程序设置的 `1e-4`，即可认为结果正确。

### 10.6 为什么结果出现两遍

`install_and_run.sh` 完成安装后会自动执行一次程序。如果随后又手动执行 `run.sh`，终端中会出现两组相同结果，这是正常现象。

---

## 11. 实验结论

本仓库使用 Microsoft SEAL CKKS 方案成功实现了单输入单输出的 `4×4` 输入、`3×3` 卷积核、步长 1、无填充的密文卷积。

直接法通过 8 次非零密文旋转完成卷积，BSGS 法将旋转偏移分解为水平小步和垂直大步，将旋转次数降低到 4 次。在固定样例和 5 组随机测试中，两种方法的解密结果均通过误差验证。

在本实验的单密文按行打包、一般稠密 `3×3` 卷积核和两级 BSGS 模型下，4 次旋转达到理论最小值。实际测试中，BSGS 法相较直接法获得约 2.250 倍加速，说明减少密文旋转和密钥切换操作能够显著提高同态卷积效率。

---

## 12. 许可证与引用

本仓库实验代码用于课程学习和实验复现。Microsoft SEAL 的使用需遵循其原项目许可证。

使用本仓库作为实验基础时，建议在报告中注明：

```text
Microsoft SEAL 4.3.2
CKKS approximate homomorphic encryption scheme
CPU implementation with C++17
```

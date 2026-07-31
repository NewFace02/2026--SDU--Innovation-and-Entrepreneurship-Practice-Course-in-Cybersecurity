# 实验三：分组密码算法实现与 SIMD 优化

| 成员 | 分工 |
|------|------|
| 宋曰琦 | 全部 |

对 AES、SM4、GIFT、TWINE 四种分组密码实现标量参考版本，并通过 T-table、AES-NI、AVX2 等指令集进行性能优化，覆盖 CTR、GCM、XTS 工作模式。

---

## 环境

| 项目 | 版本/配置 |
|------|----------|
| OS | WSL2 (Ubuntu) |
| 编译器 | GCC 13.3.0 |
| 编译选项 | `-O3 -std=c11 -march=native` |
| 指令集 | AES-NI、SSSE3、AVX2 |

---

## 目录结构

```
实验三/
├── README.md
├── Makefile
├── src/
│   ├── aes_lab.c          AES 实现与优化
│   ├── sm4_lab.c          SM4 国密算法实现与优化
│   ├── gift_lab.c         GIFT 轻量级密码实现与优化
│   └── twine_lab.c        TWINE 轻量级密码实现与优化
├── data/
│   ├── bench_16MiB.bin    基准测试输入（16 MiB）
│   └── *_benchmark_result.txt  各算法基准测试结果
├── build/                 编译产物
└── 实验报告.pdf
```

---

## 结果

### AES

| 实现方案 | 吞吐量 |
|----------|--------|
| CTR 标量参考 | 98.77 MiB/s |
| CTR 4-table | 336.29 MiB/s |
| CTR 4-table + shuffle + AVX2 | 438.81 MiB/s |
| CTR T2-table | 332.14 MiB/s |
| CTR AES-NI 单块 | 1193.14 MiB/s |
| CTR AES-NI + shuffle + AVX2 | **2142.47 MiB/s** |
| GCM CTR + AES-NI + GHASH | 29.25 MiB/s |
| XTS AES-NI + tweak | 1147.61 MiB/s |

### SM4

| 实现方案 | 吞吐量 |
|----------|--------|
| CTR 标量参考 | 112.94 MiB/s |
| CTR 4-table | 157.64 MiB/s |
| CTR T-box2 | 164.08 MiB/s |
| CTR AVX2 8-way gather | **335.98 MiB/s** |
| GCM CTR + AVX2 + 4bit GHASH | 28.07 MiB/s |
| XTS T-box2 + tweak | 190.20 MiB/s |

### GIFT & TWINE

| 算法 | 标量 全轮 | AVX2 全路径 | 瓶颈分析 |
|------|----------|-----------|----------|
| GIFT | 4.59 MiB/s | — | P-layer 比特置换（138.52 MiB/s）远慢于 S-box（825.08 MiB/s），AVX2 仅加速 S-box 无法解决 |
| TWINE | 14.63 MiB/s | 364.26 MiB/s | 标量单层 S-box（571.98）与 perm（795.66）均不占优，AVX2 nibble-slice 16 路并行带来 ~25× 提升 |

---

## 构建与运行

```bash
make all               # 编译全部四个程序
make run               # 运行 AES 基准测试
make sm4               # 运行 SM4 基准测试
make gift              # 运行 GIFT 基准测试
make twine             # 运行 TWINE 基准测试
make test              # 运行全部测试
```

---

## 参考资料

- AES: FIPS 197
- SM4: GB/T 32907-2016
- GIFT: Banik et al., CHES 2017
- TWINE: Suzaki et al., SAC 2012

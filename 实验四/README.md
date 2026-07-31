# 实验四：SM3 国密哈希算法 SIMD 并行优化

| 成员 | 分工 |
|------|------|
| 宋曰琦 | 全部 |

实现 SM3 哈希算法的标量参考版本，并通过 AVX2、AVX-512、NEON 指令集实现多路并行哈希（8/16/4 条独立消息同时计算），支持 x86-64 与 ARM64 跨平台。

---

## 环境

| 项目 | 版本/配置 |
|------|----------|
| OS | WSL2 (Ubuntu) / ARM64 |
| 编译器 | GCC |
| 编译选项 | `-O3 -std=c11 -Wall -Wextra` |
| 指令集 | AVX2、AVX-512、ARM NEON |

---

## 目录结构

```
实验四/
├── README.md
├── Makefile
├── include/
│   └── sm3.h                公共 API
├── src/
│   ├── common/
│   │   ├── sm3.c            标量参考实现
│   │   └── sm3_internal.h   内部接口
│   ├── x86/
│   │   └── sm3_x86_avx.c    AVX2 / AVX-512 并行实现
│   └── arm64/
│       └── sm3_arm64_neon.c  NEON 并行实现
├── tests/
│   └── main.c               测试 + 基准性能
└── SM3_SIMD实验报告.pdf
```

---

## 实现方案

| 实现 | 并行路数 | 平台 | 说明 |
|------|:-------:|------|------|
| scalar | 1 | 通用 | 标量参考实现 |
| AVX2 | 8 | x86-64 | `_mm256` 8 路并行压缩 |
| AVX-512 | 16 | x86-64 | `_mm512` 16 路并行压缩 |
| NEON | 4 | ARM64 | 4 路并行压缩 |

---

## 构建与运行

```bash
make                    # 编译 sm3_demo
./sm3_demo              # 运行正确性测试
./sm3_demo bench        # 运行性能基准测试
./sm3_demo hash scalar "hello"       # 标量哈希
./sm3_demo hash avx2 "hello"         # AVX2 哈希
./sm3_demo compare "hello"           # 标量 vs SIMD 对比
```

---

## 参考资料

- SM3: GB/T 32905-2016

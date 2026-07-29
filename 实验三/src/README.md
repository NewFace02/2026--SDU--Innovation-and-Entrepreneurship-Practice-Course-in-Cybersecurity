# Source map

The current implementation is kept in one C translation unit so it is easy to
compile for the report, but the code is arranged by function family:

- AES reference core: scalar key schedule, SubBytes, ShiftRows, MixColumns.
- T-table optimization: 4-table AES and T2-table AES.
- instruction-set optimization: AES-NI, SSSE3 `pshufb`, AVX2 `_mm256_*`.
- mode optimization: CTR batching, GCM 4-bit GHASH table, XTS tweak update.
- benchmark and vectors: file loading, correctness checks, throughput tests.

If this assignment is later expanded, these groups can be split into
`aes_basic.c`, `aes_ttable.c`, `aes_ni.c`, `modes.c`, `bench.c`, and `main.c`.

SM4 is implemented separately in `sm4_lab.c`:

- scalar SM4 reference implementation and standard test vector;
- 4-table T-box implementation;
- PPT-style T-box2 implementation using `T0[x] || T0[x]` in a 64-bit word to
  read rotated 32-bit values by offset instead of emitting rotate operations;
- AVX2 8-way CTR implementation using `_mm256_i32gather_epi32`;
- SM4-GCM using AVX2 CTR plus 4-bit GHASH table;
- SM4-XTS using T-box2 block encryption/decryption and tweak update.

GIFT/TWINE optimization code is split into `gift_lab.c` and `twine_lab.c`:

- TWINE scalar 36-round path and AVX2 16-block path;
- TWINE register layout: `x[0]..x[15]` are 16 nibble positions across 16
  blocks, so the hot S-box operation maps directly to `vpshufb`;
- GIFT scalar 28-round path and AVX2 S-box kernel;
- GIFT hotspot observation verified by segment timing: `vpshufb` makes the
  4-bit S-box cheap, but the scalar bit permutation layer is the bottleneck.
- TWINE hotspot observation verified by segment timing: S-box and nibble
  permutation are not individually dominant; the AVX2 speedup comes from
  16-block parallelism and keeping the round state in nibble-sliced ymm registers.

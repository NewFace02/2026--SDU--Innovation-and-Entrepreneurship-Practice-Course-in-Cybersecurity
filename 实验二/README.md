# 实验二：ECDSA 摘要未绑定时的存在性伪造（Digest-Forgery）

## 一、实验目的

验证一个常见的 **API/协议误用** 场景：如果验证方允许攻击者在验证时**自行提供摘要 `e`**（而不是验证方自己对一条确定消息计算 `SHA256(message)`），那么攻击者仅凭公钥 `P`，**不知道私钥**，也能构造出一组同时通过验证方程的 `(e', r', s')`。

**需要特别强调的边界（实验的核心结论）**：

- 这**不是**对 ECDSA 签名算法本身的破解；标准 ECDSA 的不可伪造性（在 `e = H(m)` 且 `H` 为安全哈希函数的前提下）不受影响。
- 攻击者**无法**针对一条**事先指定**的目标消息（如“Authorize payment of 1 BTC”）伪造出有效签名——伪造出的摘要 `e'` 几乎不可能等于 `SHA256(目标消息)`。
- 该实验揭示的是：**验证逻辑必须自己对消息计算摘要，绝不能信任调用方传入的摘要**。这是真实系统中出现过的实现漏洞类别（例如允许外部传入“已经算好的哈希”）。

## 二、数学原理

ECDSA 验证方程为：给定签名 `(r, s)` 和摘要 `e`，计算

```
w = s^-1 mod n
R' = (e·w)·G + (r·w)·P
```

若 `R'.x mod n == r`，则验证通过。

关键观察：**验证方程只使用 `e`、`r`、`s`、`P`，不使用私钥**。因此可以反过来构造：

1. 攻击者任选两个标量 `u, v`（`v ≠ 0`），计算 `R' = u·G + v·P`，令 `r' = R'.x mod n`；
2. 令 `s' = r' · v^-1 mod n`，`e' = r' · u · v^-1 mod n`；
3. 代入验证方程可得 `w' = s'^-1 = v·r'^-1`，`R' = (e'·w')G + (r'·w')P = u·G + v·P`，与构造时的 `R'` 完全一致，因此 `(e', r', s')` 必定通过验证——且全程未使用私钥。

这正是 `ecdsa_digest_forgery.py` 中 `forge_for_chosen_digest()` 的实现。

## 三、代码说明

`ecdsa_digest_forgery.py` 是一份**教学用途的独立实现**（不依赖第三方密码学库），包含：

- `secp256k1` 曲线上的仿射点加法与标量乘法（`point_add` / `scalar_mul`）；
- 标准 ECDSA 签名与验证（`sign_digest` / `verify_digest`）；
- 上述摘要伪造构造（`forge_for_chosen_digest`）；
- `main()` 中的完整实验流程：生成密钥对 → 对真实消息签名并验证 → 攻击者在不知私钥的情况下伪造 `(e', r', s')` → 分别验证“伪造摘要”与“目标消息摘要”两种场景。

程序只使用 Python 3 标准库（`dataclasses`、`hashlib`），无需安装任何依赖。

## 四、运行方法

```bash
python3 ecdsa_digest_forgery.py
```

## 五、实验结果

完整输出见 [`运行结果.txt`](运行结果.txt)，核心结果摘录：

```
legitimate verify                = PASS
verification equation R'=uG+vP = PASS
verify chosen digest e'         = PASS
verify target message hash      = FAIL (expected)
```

解读：

| 验证项 | 结果 | 含义 |
| --- | --- | --- |
| 合法签名验证 | PASS | 私钥持有者的正常签名可以正确验证，作为对照组 |
| `R' = uG + vP` 构造方程 | PASS | 伪造点的构造过程本身自洽 |
| 用伪造摘要 `e'` 验证 `(r', s')` | PASS | **攻击者在不知私钥的情况下，构造出了一组通过验证的 `(摘要, 签名)` 对** |
| 用目标消息真实摘要验证 `(r', s')` | FAIL（预期结果） | 攻击者无法伪造对“指定消息”的签名，ECDSA 本身未被攻破 |

## 六、报告

完整实验报告见 [`报告/实验二报告-ECDSA摘要伪造实验.pdf`](报告/实验二报告-ECDSA摘要伪造实验.pdf)（原 LaTeX 源码及编译中间文件未随仓库分发，如需重新编译可自行用 XeLaTeX 处理同名 `.tex` 源文件）。

## 七、依赖说明

实验过程中审计参考了 Bitcoin Core 官方 `secp256k1` 仓库（用于比对参数与实现细节），因体积较大且为第三方代码，**未纳入本仓库**。如需查阅，请参见官方仓库：https://github.com/bitcoin-core/secp256k1 。本实验的曲线参数（`p`、`n`、生成元 `G`）均已在 `ecdsa_digest_forgery.py` 顶部显式给出，无需该仓库即可独立运行与复现。

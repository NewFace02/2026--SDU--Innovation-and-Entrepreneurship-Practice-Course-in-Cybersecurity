# 实验一：Bitcoin Testnet4 交易构造、区块编码与 PoW 独立验证

小组成员：宋佳原、宋曰琦、杜昊霖

## 一、实验目的

1. 在 Bitcoin Testnet4 上构造、签名并广播一笔原生 SegWit（P2WPKH）交易，获取其 raw hex。
2. **不依赖 Bitcoin Core 的解码接口**，自行按字节 / 按比特解析 Transaction、Input、Output、锁定脚本与解锁数据（witness）。
3. 选取一个已确认的 Testnet4 区块，解析其 80 字节区块头与全部交易（含 Coinbase），独立重算 txid、wtxid、Merkle root、witness commitment 与区块哈希，并验证工作量证明（PoW）。

## 二、实验环境

| 项目 | 配置 |
| --- | --- |
| 操作系统 | Ubuntu 20.04（Windows/WSL） |
| Bitcoin Core | 31.1，网络参数 `-testnet4` |
| 节点模式 | 裁剪节点，`prune=1000` |
| 辅助工具 | `bitcoin-cli`、Python 3、`jq`、`xxd` |

## 三、目录结构

```
实验一/
├── README.md                本文件
├── parse_tx.py               交易独立解析程序（只用 Python 标准库）
├── parse_block.py            区块 / Merkle root / PoW 独立验证程序
├── 实验数据/                 实验各阶段产生的原始数据与解析结果
└── 截图/                     实验过程关键截图（编号对应实验步骤）
```

报告 PDF（`bitcoin_testnet_report.pdf`，原 LaTeX 源码及编译中间文件已不再随仓库分发）中的完整叙述、公式推导与图表，均可依据下文的“核对索引”在本目录中找到对应的原始证据文件。

## 四、核心结论（结果一览）

| 检查项 | 结果 |
| --- | --- |
| 交易构造、签名、广播 | 通过，`testmempoolaccept.allowed=true` |
| 独立解析交易字节数 | 222/222，全部消费 |
| 独立重算 txid / wtxid | 与 Bitcoin Core 完全一致 |
| 区块字节数 | 2083/2083，全部消费，8 笔交易边界正确 |
| Coinbase BIP34 高度解析 | 145319，与区块高度一致 |
| Merkle root 独立重算 | 与区块头一致 |
| Witness commitment 独立重算 | 与 Coinbase OP_RETURN 中的承诺一致 |
| PoW（`H ≤ target`） | 通过 |

程序化验证结果详见 [`实验数据/14-区块验证结果.txt`](实验数据/14-区块验证结果.txt)：

```
All Block Bytes Consumed: PASS
Transaction Count: PASS
Block Hash: PASS
Proof Of Work: PASS
Merkle Root: PASS
Witness Commitment: PASS
Coinbase First: PASS
```

## 五、如何复现

```bash
# 1) 独立解析本实验构造的交易，并核对 txid / wtxid
python3 parse_tx.py 实验数据/05-已签名交易.hex \
  --expected-txid dea08b1b3e8595314a7affd5e0299b81537c48eb78b8634d317f0c4f44011d5f \
  --expected-wtxid df9cc5342bca17c969b665ae32f133dd1d0d86b6b8e09fa0c4ce41898f416a29 \
  --prefix /tmp/12-experiment-tx

# 2) 独立解析 Coinbase 交易
python3 parse_tx.py 实验数据/11-Coinbase原始数据.hex \
  --expected-txid 2526d3c43f26d36cb7683e0f3c5c369ff2819cd3316f70f1d893b26159338473 \
  --expected-wtxid 2202b18db31e3b6d9d16ad9afc8a349f250205e9b601261a8f762c49b3ab3939 \
  --prefix /tmp/13-coinbase

# 3) 独立解析整个区块，重算 Merkle root / witness commitment / PoW
python3 parse_block.py 实验数据/10-区块原始数据.hex \
  --expected-hash 000000000012057bbfe9e5ce7979a8fe06c37ac350808187a97bcce8635404bf \
  --prefix /tmp/14-block
```

两个脚本只依赖 Python 3 标准库，无需安装依赖或运行 Bitcoin Core 节点即可复现解析与验证过程。

## 六、实验数据说明（`实验数据/` 目录，按实验步骤编号）

| 编号 | 文件 | 内容 |
| --- | --- | --- |
| 01 | `01-UTXO列表.json` | Faucet 提供的初始 UTXO |
| 02 | `02-目标地址.txt` | 交易目标接收地址 |
| 03 | `03-未签名交易.hex` / `.json` | `createrawtransaction` 构造的未签名交易 |
| 04 | `04-资金补全交易.hex/.json`、`04-资金补全结果.json`、`04-阶段摘要.json` | `fundrawtransaction` 补全找零与手续费后的交易 |
| 05 | `05-已签名交易.hex/.json`、`05-签名结果.json`、`05-阶段摘要.json` | 钱包签名后的最终交易（222 字节，本实验的核心研究对象） |
| 06 | `06-mempool预检查.json` | `testmempoolaccept` 广播前预验证结果 |
| 07 | `07-交易ID.txt`、`07-mempool条目.json`、`07-网络交易记录.json`、`07-钱包交易记录.json` | 广播后 txid 与 mempool / 网络 / 钱包视角的交叉核对 |
| 08 | `08-确认状态.json` | 交易确认状态查询 |
| 09 | `09-区块哈希.txt` | 后续用于区块解析实验的已确认区块哈希（高度 145319） |
| 10 | `10-区块原始数据.hex`、`10-区块头原始数据.hex`、`10-区块解码.json`、`10-区块头解码.json`、`10-区块RPC输出.json`、`10-区块摘要.json` | 所选已确认区块的原始数据及 Bitcoin Core 解码结果（用于交叉对照） |
| 11 | `11-Coinbase原始数据.hex`、`11-Coinbase解码.json`、`11-Coinbase十六进制转储.txt` | 区块内 Coinbase 交易 |
| 12 | `12-实验交易解析结果.json`、`12-实验交易字节表.tsv` | `parse_tx.py` 对本实验交易的逐字节 / 逐比特解析结果 |
| 13 | `13-Coinbase解析结果.json`、`13-Coinbase字节表.tsv` | `parse_tx.py` 对 Coinbase 交易的逐字节解析结果 |
| 14 | `14-区块解析结果.json`、`14-区块头字节表.tsv`、`14-区块验证结果.txt` | `parse_block.py` 对整个区块的解析与 PoW/Merkle/commitment 验证结果 |

> 说明：**广播交易（05）与用于区块/PoW 实验的已确认区块（09-14）是两个不同的研究对象**——实验时间窗口内自建交易尚未被打包确认，因此区块与 PoW 部分选用另一个已确认的 Testnet4 区块独立完成，报告与本 README 均未将该区块描述为自建交易所在区块。

## 七、截图说明（`截图/` 目录）

编号与上述实验步骤及报告图表一一对应，文件名已直接说明截图内容，助教可不打开 PDF 报告直接按顺序浏览：

01 节点同步完成 → 02 Faucet UTXO → 03 未签名交易 → 04A/04B 找零与手续费 → 05 手续费核对 → 06 金额守恒 → 07 钱包签名 → 08 txid/wtxid 分离 → 09 witness 栈 → 10 输出脚本 → 11 mempool 预验证 → 12 广播一致性 → 13 广播后 mempool 状态 → 14 公共浏览器记录 → 15-18 区块选择与基本信息 → 19-23 Coinbase 结构与区块交易列表 → 24-27 实验交易逐字节/逐比特解析 → 28-29 Coinbase 独立解析结果。

## 八、关键原理速览

- **SegWit 交易序列化**：`version‖marker‖flag‖vin‖vout‖witness‖locktime`；固定宽度整数小端编码，数组/脚本长度用 CompactSize；txid 排除 marker/flag/witness，wtxid 对完整序列化计算，两者均为 SHA256d 后反转字节序。
- **P2WPKH 锁定脚本**：`00 14 <20字节 HASH160(pubkey)>`；解锁数据（签名 + 公钥）位于 witness 栈，`scriptSig` 必须为空。
- **区块头（80 字节）**：version(4) + previous block hash(32) + Merkle root(32) + timestamp(4) + nBits(4) + nonce(4)，均为小端编码，哈希字段为内部字节序。
- **PoW**：`target = coefficient × 256^(exponent-3)`（由 nBits 拆分），验证 `SHA256d(header) 的整数值 ≤ target`。

## 九、参考资料

1. Bitcoin Developer Reference — Transactions: https://developer.bitcoin.org/reference/transactions.html
2. Bitcoin Developer Reference — Block Chain: https://developer.bitcoin.org/reference/block_chain.html
3. BIP 34, Block v2, Height in Coinbase: https://bips.dev/34/
4. BIP 141, Segregated Witness: https://bips.dev/141/
5. BIP 143, Transaction Signature Verification for Version 0 Witness Program: https://bips.dev/143/
6. BIP 94, Testnet 4: https://bips.dev/94/

# 实验二：ECDSA 摘要未绑定时的存在性伪造

## 运行

```bash
python3 ecdsa_digest_forgery.py
```

程序只依赖 Python 3 标准库，直接实现 secp256k1 曲线点运算、ECDSA 签名与验证，
并按课件公式构造 `(e', r', s')`。

关键预期结果：

```text
legitimate verify                = PASS
verification equation R'=uG+vP = PASS
verify chosen digest e'         = PASS
verify target message hash      = FAIL (expected)
```

这说明攻击者可以同时选择摘要和签名，使验证方程成立；它不能伪造预先指定消息，
因此不是对 ECDSA 不可伪造性的破解。

## 文件

- `ecdsa_digest_forgery.py`：实验代码。
- `forgery_result.txt`：已运行结果。
- `ecdsa_forgery_report.tex`：实验报告。
- `ecdsa_forgery_report.pdf`：XeLaTeX 编译结果。
- `secp256k1/`：实验时审计的 Bitcoin Core 官方仓库浅克隆。

报告使用 XeLaTeX 编译：

```bash
xelatex ecdsa_forgery_report.tex
xelatex ecdsa_forgery_report.tex
```


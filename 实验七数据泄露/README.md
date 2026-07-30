# 实验七 数据泄露安全测评

使用 **Garak 0.16.0** 对 **Qwen2.5-0.5B-Instruct** 进行全方位安全测评。

## 环境

- WSL2 (Ubuntu) + conda `dlcompilerattack_llm`
- PyTorch 2.6.0 (CUDA 12.4) / GPU: NVIDIA RTX 3060 (6GB)

## 模型

[Qwen2.5-0.5B-Instruct](https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct)，阿里通义千问系列最小指令模型，0.5B 参数。

## 启动

```bash
bash run_round1_leakreplay.sh    # 训练数据记忆化
bash run_round2_jailbreak_pii.sh # 越狱 + PII + 提示词提取
```

## 结果

| # | 探针 | 测试数 | ASR |
|---|------|--------|-----|
| 1 | GuardianComplete（卫报） | 9 | 0% |
| 2 | LiteratureComplete（文学） | 256 | 0% |
| 3 | NYTComplete（纽约时报） | 30 | 0% |
| 4 | PotterComplete（哈利波特） | 30 | 0% |
| 5 | Ablation_Dan_11_0（DAN越狱） | 127 | **99.2%** |
| 6 | DanInTheWild（野外越狱） | 256 | **62.5%** |
| 7 | LatentJailbreak（潜伏注入） | 64 | **42.2%** |
| 8 | PIILeakTwin（PII泄露） | 81 | **9.9%** |
| 9 | PIILeakTriplet（PII三元组） | 4 | 0% |
| 10 | SystemPromptExtraction（提示词提取） | 64 | **12.5%** |

> ASR = 攻击成功率。数据按 seed=42, generations=1 测得。

**结论：Qwen2.5-0.5B 未发现训练数据记忆化，但安全护栏几乎为零，越狱攻击成功率最高 99.2%，PII 和提示词也存在泄露风险。**

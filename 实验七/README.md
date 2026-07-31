# 实验七：基于 Garak 的大语言模型安全测评

| 成员 | 分工 |
|------|------|
| 宋佳原 | 注入攻击 |
| 宋曰琦 | 数据泄露 |
| 杜昊霖 | 恶意代码生成 |

使用 [Garak](https://github.com/NVIDIA/garak)，针对 **Qwen2.5-0.5B-Instruct** 进行三维度安全测评。

---

## 环境

| 项目 | 版本/配置 |
|------|----------|
| OS | WSL2 (Ubuntu) |
| Python | 3.10+ |
| Garak | 0.15 / 0.16 |
| PyTorch | 2.6.0 (CUDA 12.4) |
| GPU | RTX 3060 (6GB) / RTX 4050 (6GB) |
| 模型 | Qwen2.5-0.5B-Instruct |

---

## 结果

### 注入攻击（8 个探针，10,240 次测试）

| 攻击类型 | 代表探针 | ASR |
|----------|----------|-----|
| 间接注入 | LatentInjectionTranslationEnFr | 29.53% |
| 间接注入 | LatentInjectionReport | 19.92% |
| 间接注入 | LatentInjectionResume | 8.44% |
| 直接劫持 | HijackHateHumans | **66.09%** |
| 直接劫持 | HijackKillHumans | 36.64% |
| 直接劫持 | HijackLongPrompt | 32.66% |
| **总体** | — | **24.83%** |

### 数据泄露（10 个探针，921 次测试）

| 类别 | 代表探针 | 测试数 | ASR |
|------|----------|:-----:|-----|
| 训练数据记忆化 | GuardianComplete | 9 | 0% |
| 训练数据记忆化 | LiteratureComplete | 256 | 0% |
| 训练数据记忆化 | NYTComplete | 30 | 0% |
| 训练数据记忆化 | PotterComplete | 30 | 0% |
| 越狱 | Ablation_Dan_11_0 | 127 | **99.2%** |
| 越狱 | DanInTheWild | 256 | 62.5% |
| 越狱 | LatentJailbreak | 64 | 42.2% |
| PII 泄露 | PIILeakTwin | 81 | 9.9% |
| PII 泄露 | PIILeakTriplet | 4 | 0% |
| 提示词提取 | SystemPromptExtraction | 64 | 12.5% |

### 恶意代码生成（4 类探针，720 次测试）

| 探针类别 | 有效生成 | 自动命中 | ASR |
|----------|:-------:|:-------:|-----|
| TopLevel | 48 | 15 | 31.25% |
| Payload | 360 | 172 | 47.78% |
| SubFunctions | 168 | 160 | **95.24%** |
| Evasion | 144 | 74 | 51.39% |
| **总体** | **720** | **421** | **58.47%** |

---

## 结论

Qwen2.5-0.5B 未发现训练数据记忆化，但安全护栏几乎为零：越狱攻击成功率最高 99.2%，恶意代码子功能生成 ASR 达 95.24%，直接提示劫持 ASR 达 66.09%。小规模模型对三类攻击均缺乏有效抵抗。

---

## 参考资料

- Garak: https://github.com/NVIDIA/garak
- Qwen2.5-0.5B-Instruct: https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct
- OWASP LLM Top 10: https://owasp.org/www-project-top-10-for-large-language-model-applications/

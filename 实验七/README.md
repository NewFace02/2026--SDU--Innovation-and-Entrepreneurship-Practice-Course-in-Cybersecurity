# 实验七：基于 Garak 的大语言模型安全测评

小组成员：宋佳原、宋曰琦、杜昊霖

使用开源 AI 安全测评工具 [Garak](https://github.com/NVIDIA/garak)，针对开源大语言模型 **Qwen2.5-0.5B-Instruct**，从三个维度进行全方位安全测评：

- **注入攻击**（宋佳原）— 间接/潜伏注入 + 直接提示劫持
- **数据泄露**（宋曰琦）— 训练数据记忆化 + 越狱/PII/提示词提取
- **恶意代码生成**（杜昊霖）— malwaregen 全量探针

---

## 环境

- WSL2 (Ubuntu) / Linux
- Python 3.10+，Garak 0.15/0.16
- PyTorch + CUDA / GPU（RTX 3060/4050 6GB）
- 模型：[Qwen2.5-0.5B-Instruct](https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct)

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

### 数据泄露（10 个探针）

| 类别 | 代表探针 | ASR |
|------|----------|-----|
| 训练数据记忆化 | GuardianComplete / LiteratureComplete / NYTComplete | 0% |
| 越狱 | Ablation_Dan_11_0 | **99.2%** |
| 越狱 | DanInTheWild | 62.5% |
| 越狱 | LatentJailbreak | 42.2% |
| PII 泄露 | PIILeakTwin | 9.9% |
| 提示词提取 | SystemPromptExtraction | 12.5% |

### 恶意代码生成（4 类探针，240 提示 × 3 次生成 = 720）

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

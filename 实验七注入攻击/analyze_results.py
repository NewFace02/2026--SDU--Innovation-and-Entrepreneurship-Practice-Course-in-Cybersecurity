import json
import sys
from collections import defaultdict

report_paths = sys.argv[1:] if len(sys.argv) > 1 else [
    "/home/sjy/.local/share/garak/garak_runs/qwen05b_promptinjection.report.jsonl",
    "/home/sjy/.local/share/garak/garak_runs/qwen05b_promptinject_direct.report.jsonl",
]

by_uuid = defaultdict(list)
evals = []

for report_path in report_paths:
    with open(report_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rec = json.loads(line)
            except json.JSONDecodeError:
                continue
            et = rec.get("entry_type")
            if et == "attempt":
                by_uuid[rec["uuid"]].append(rec)
            elif et == "eval":
                evals.append(rec)

# keep only the final (post-evaluation) record per attempt uuid
finals = []
for uuid, recs in by_uuid.items():
    recs_with_dr = [r for r in recs if r.get("detector_results")]
    finals.append(recs_with_dr[-1] if recs_with_dr else recs[-1])

print(f"total unique attempts: {len(finals)}")
print(f"total eval summary records: {len(evals)}\n")

print(f"{'Probe':50s} {'Detector':30s} {'ASR%':>7s} {'hits/n':>12s}")
print("-" * 105)
overall_hits = 0
overall_total = 0
rows = []
for e in evals:
    probe = e.get("probe")
    detector = e.get("detector")
    total = e.get("total_evaluated", 0)
    hits = e.get("fails", 0)
    asr = (hits / total * 100) if total else 0.0
    overall_hits += hits
    overall_total += total
    rows.append({"probe": probe, "detector": detector, "asr": asr, "n": total, "hits": hits})
    print(f"{probe:50s} {detector:30s} {asr:6.1f}% {hits:5d}/{total:<5d}")

print("-" * 105)
overall_asr = (overall_hits / overall_total * 100) if overall_total else 0.0
print(f"{'OVERALL':50s} {'':30s} {overall_asr:6.1f}% {overall_hits:5d}/{overall_total:<5d}")

with open("/home/sjy/garak-eval/summary.json", "w", encoding="utf-8") as f:
    json.dump(
        {"rows": rows, "overall_hits": overall_hits, "overall_total": overall_total, "overall_asr": overall_asr},
        f, ensure_ascii=False, indent=2,
    )

# ---- extract concrete hit examples (one per probe) for the case-study section ----
print("\n\n=== case study candidates (one hit example per probe) ===")
seen_probes = set()
case_studies = []
for rec in finals:
    probe = rec.get("probe_classname")
    if probe in seen_probes:
        continue
    dr = rec.get("detector_results", {})
    hit_idx = None
    hit_detector = None
    for det, scores in dr.items():
        for i, s in enumerate(scores):
            if s and s > 0:
                hit_idx = i
                hit_detector = det
                break
        if hit_idx is not None:
            break
    if hit_idx is None:
        continue
    prompt_text = rec["prompt"]["turns"][0]["content"]["text"]
    output_text = rec["outputs"][hit_idx]["text"]
    case_studies.append({
        "probe": probe,
        "detector": hit_detector,
        "prompt": prompt_text,
        "output": output_text,
    })
    seen_probes.add(probe)
    print(f"--- {probe} (detector: {hit_detector}) ---")
    print("PROMPT:", prompt_text[:300].replace("\n", " "))
    print("OUTPUT:", output_text[:300].replace("\n", " "))
    print()

with open("/home/sjy/garak-eval/case_studies.json", "w", encoding="utf-8") as f:
    json.dump(case_studies, f, ensure_ascii=False, indent=2)

print(f"\nwrote summary.json ({len(rows)} rows) and case_studies.json ({len(case_studies)} examples)")

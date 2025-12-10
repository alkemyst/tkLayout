#!/usr/bin/env python3

import csv
import sys
import math
from collections import defaultdict

# ---------------------------------------------------------
# CONFIGURATION
# ---------------------------------------------------------
START = 20_000          # 20 kGy
STOP  = 1_600_000       # 1.5 MGy
STEP  = 10 ** 0.1       # geometric step

# ---------------------------------------------------------
# READ INPUT
# ---------------------------------------------------------
input_file = sys.argv[1]

rows = []
with open(input_file, newline="") as f:
    reader = csv.DictReader(f)
    # Normalize names
    reader.fieldnames = [name.strip() for name in reader.fieldnames]

    for r in reader:
        # Clean row (strip spaces)
        r = {k.strip(): (v.strip() if isinstance(v, str) else v) for k, v in r.items()}

        # Convert region keys
        layer = int(r["Layer"]) if "Layer" in r and r["Layer"] else None
        ring  = int(r["Ring"])  if "Ring" in r and r["Ring"] else None

        sec = r["Section"]

        if sec in ("TB2S", "TBPS"):
            region = f"{sec}_{layer}"
        else:
            region = f"{sec}_{ring}"

        # Find sensorsDoseMax_Gy column
        dose_key = None
        for k in r:
            if k.lower() == "sensorsdosemax_gy":
                dose_key = k
                break
        if dose_key is None:
            raise RuntimeError(f"No sensorsDoseMax_Gy column found. Columns: {list(r.keys())}")

        dose = float(r[dose_key])

        rows.append({"region": region, "dose": dose})

# ---------------------------------------------------------
# BUILD RADIATION THRESHOLD GRID
# ---------------------------------------------------------
thresholds = []
x = START
while x <= STOP:
    thresholds.append(x)
    x *= STEP

# ---------------------------------------------------------
# COUNT MODULES PER REGION PER THRESHOLD
# ---------------------------------------------------------
region_counts = {t: defaultdict(int) for t in thresholds}
global_counts = {t: 0 for t in thresholds}

regions = sorted(set(r["region"] for r in rows))

for r in rows:
    region = r["region"]
    dose   = r["dose"]

    for t in thresholds:
        if dose > t:
            region_counts[t][region] += 1
            global_counts[t] += 1

# ---------------------------------------------------------
# WRITE OUTPUT CSV
# ---------------------------------------------------------
output_file = "irradiation_bins.csv"

# Count total modules per region
region_total_modules = defaultdict(int)
for r in rows:
    region_total_modules[r["region"]] += 1

with open(output_file, "w", newline="") as f:
    writer = csv.writer(f)

    # Header row
    header = ["Region\\Max_Irrad"] + [f"{t:.1f}" for t in thresholds] + ["Total"]
    writer.writerow(header)

    # Region rows
    for region in regions:
        row = [region]
        for t in thresholds:
            row.append(region_counts[t][region])

        # Correct total = number of modules in this region
        row.append(region_total_modules[region])
        writer.writerow(row)

    # Extra row: totals across all regions
    total_row = ["ALL"]
    for t in thresholds:
        total_row.append(global_counts[t])

    # Total modules across all regions
    total_row.append(len(rows))

    writer.writerow([])
    writer.writerow(total_row)

print(f"Wrote {output_file}")

#!/usr/bin/env python3

import csv

import sys

# Read input CSV file passed as first argument
input_file = sys.argv[1]
rows = []
with open(input_file, newline="") as f:
    reader = csv.DictReader(f)
    # Normalize column names: strip spaces
    reader.fieldnames = [name.strip() for name in reader.fieldnames]
    for r in reader:
        # Strip leading/trailing spaces from all keys and values
        #r = {k.strip(): (v.strip() if isinstance(v, str) else v) for k, v in r.items()}
        # Strip spaces from keys of each row
        #r = {k.strip(): v for k, v in r.items()}
        # Convert numeric fields as needed
        # Some sections (TEDD) may not have Layer; some may not have Ring
        if "Layer" in r and r["Layer"]:
            r["Layer"] = int(r["Layer"])
        else:
            r["Layer"] = None
        if "Ring" in r and r["Ring"]:
            r["Ring"] = int(r["Ring"])
        else:
            r["Ring"] = None
        # Handle possible alternative column names
        dose_key = None
        for k in r.keys():
            if k.lower() == "sensorsdosemax_gy":
                dose_key = k
                break
        if dose_key is None:
            raise KeyError(f"Could not find sensorsDoseMax_Gy column. Columns present: {list(r.keys())}")

        r["sensorsDoseMax_Gy"] = float(r[dose_key])
        rows.append(r)
(r)

# Determine region and collect max dose per region
region_max = {}
for r in rows:
    sec = r["Section"]
    if sec in ("TB2S", "TBPS"):
        region = f"{sec}_{r['Layer']}"
    else:  # TEDD_1 or TEDD_2
        region = f"{sec}_{r['Ring']}"

    dose = r["sensorsDoseMax_Gy"]
    if region not in region_max or dose > region_max[region]:
        region_max[region] = dose

# Write CSV
with open("region_max_dose.csv", "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["section", "region", "max_sensorsDoseMax_Gy"])
    for region, dose in region_max.items():
        section = region.split("_")[0]
        writer.writerow([section, region, dose])

#!/usr/bin/env python3
import argparse
import csv
import glob
import pathlib
import re


def parse_args():
    p = argparse.ArgumentParser(description="Convert MOOSE SideValueSampler CSVs into x,y surface profiles.")
    p.add_argument("pattern", help="Pattern for raw VPP CSVs, e.g. outputs/2d/run/rz_surface_profile_surface_top_*.csv")
    p.add_argument("--outdir", default=None, help="Output directory for x,y CSVs; defaults next to the raw files.")
    p.add_argument("--sort", choices=("x", "y", "id"), default="x", help="Sort key for the exported profile.")
    p.add_argument("--stem", default="surface_xy", help="Prefix for exported x,y CSVs.")
    return p.parse_args()


def read_time_map(path):
    if not path.exists():
        return {}
    with path.open(newline="") as f:
        return {int(row["timestep"]): float(row["time"]) for row in csv.DictReader(f)}


def timestep_from_name(path):
    m = re.search(r"_(\d+)\.csv$", path.name)
    if not m:
        raise ValueError(f"Could not parse timestep from {path}")
    return int(m.group(1))


def main():
    args = parse_args()
    files = [pathlib.Path(p) for p in sorted(glob.glob(args.pattern)) if not p.endswith("_time.csv")]
    if not files:
        raise SystemExit(f"No files matched: {args.pattern}")

    outdir = pathlib.Path(args.outdir) if args.outdir else files[0].parent / f"{args.stem}_profiles"
    outdir.mkdir(parents=True, exist_ok=True)
    time_map = read_time_map(pathlib.Path(args.pattern.replace("*", "time")))

    for raw in files:
        with raw.open(newline="") as f:
            rows = list(csv.DictReader(f))
        rows.sort(key=lambda row: float(row[args.sort]))
        step = timestep_from_name(raw)
        t = time_map.get(step)
        suffix = f"_t{t:.6f}" if t is not None else ""
        out = outdir / f"{args.stem}_{step:04d}{suffix}.csv"
        with out.open("w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["x", "y"])
            for row in rows:
                writer.writerow([row["x"], row["y"]])
        print(out)


if __name__ == "__main__":
    main()

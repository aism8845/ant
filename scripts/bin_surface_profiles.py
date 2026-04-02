#!/usr/bin/env python3
import argparse
import os
import re
import shutil
import sys
import time
from pathlib import Path


ROUND_RE = re.compile(r"^(?P<stem>.+)_(?P<round>\d{4,})\.csv$")


def move_completed_rounds(root: Path, prefix: str) -> int:
    moved = 0
    files_by_round = {}

    for path in root.glob(f"{prefix}_surface_line_*_*.csv"):
        name = path.name
        if name.endswith("_time.csv"):
            continue
        match = ROUND_RE.match(name)
        if not match:
            continue
        round_id = match.group("round")
        files_by_round.setdefault(round_id, []).append(path)

    for round_id, files in files_by_round.items():
        if len(files) < 10:
            continue
        target_dir = root / f"profiles_round_{round_id}"
        target_dir.mkdir(exist_ok=True)
        for src in files:
            dst = target_dir / src.name
            if dst.exists():
                continue
            shutil.move(str(src), str(dst))
            moved += 1

    return moved


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("run_dir")
    parser.add_argument("--prefix", default="puck_surface_profile")
    parser.add_argument("--poll-seconds", type=float, default=5.0)
    parser.add_argument("--idle-exit-seconds", type=float, default=120.0)
    args = parser.parse_args()

    run_dir = Path(args.run_dir).resolve()
    if not run_dir.is_dir():
        print(f"run dir not found: {run_dir}", file=sys.stderr)
        return 2

    last_change = time.time()
    while True:
        moved = move_completed_rounds(run_dir, args.prefix)
        if moved:
            last_change = time.time()

        solver_log = run_dir / "solver.log"
        finished = False
        if solver_log.exists():
            try:
                tail = solver_log.read_text(errors="ignore")[-4000:]
                if (
                    "Finished Executing" in tail
                    or "MPI_Abort" in tail
                    or "Solve failed and timestep already at or below dtmin" in tail
                ):
                    finished = True
            except OSError:
                pass

        if finished and time.time() - last_change > args.idle_exit_seconds:
            move_completed_rounds(run_dir, args.prefix)
            return 0

        time.sleep(args.poll_seconds)


if __name__ == "__main__":
    raise SystemExit(main())

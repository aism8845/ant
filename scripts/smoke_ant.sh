#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${ROOT}"

EXE="${EXE:-./ant-opt}"
INPUT="${SMOKE_INPUT:-inputs/current/smoke.i}"
BUILD="${BUILD:-1}"
END_TIME="${SMOKE_END_TIME:-0.02}"
JAC_END_TIME="${SMOKE_JAC_END_TIME:-0.004}"
JAC_DT="${SMOKE_JAC_DT:-0.002}"
JAC_THRESH="${SMOKE_JAC_THRESHOLD:-1e-6}"

mkdir -p outputs

if [[ "${BUILD}" == "1" ]]; then
  echo "[smoke] build"
  make -j8
fi

echo "[smoke] jacobian spot-check"
"${EXE}" -i "${INPUT}" \
  Executioner/end_time="${JAC_END_TIME}" \
  Executioner/TimeStepper/type=ConstantDT \
  Executioner/TimeStepper/dt="${JAC_DT}" \
  Executioner/petsc_options='-snes_converged_reason -ksp_converged_reason -snes_test_jacobian' \
  Outputs/exodus=false \
  > outputs/ant_smoke_jac.log 2>&1

rg -n "snes_test_jacobian|\\|\\|J - Jfd\\|\\||CONVERGED|DIVERGED" outputs/ant_smoke_jac.log || true

if rg -q "DIVERGED|Solve Did NOT Converge" outputs/ant_smoke_jac.log; then
  echo "[smoke] jacobian run diverged" >&2
  exit 1
fi

python3 - <<'PY' "${JAC_THRESH}" outputs/ant_smoke_jac.log
import re
import sys
thresh = float(sys.argv[1])
text = open(sys.argv[2], encoding='utf-8', errors='ignore').read()
matches = re.findall(r"\|\|J - Jfd\|\|_F\s*/\s*\|\|J\|\|_F = ([0-9.eE+-]+)", text)
if not matches:
    print("[smoke] jacobian ratio not found", file=sys.stderr)
    sys.exit(1)
ratio = float(matches[-1])
print(f"[smoke] jacobian ratio = {ratio:.6e}")
if ratio > thresh:
    print(f"[smoke] jacobian ratio exceeds threshold {thresh:.6e}", file=sys.stderr)
    sys.exit(1)
PY

echo "[smoke] transient run"
"${EXE}" -i "${INPUT}" \
  Executioner/end_time="${END_TIME}" \
  Executioner/petsc_options='-snes_converged_reason -ksp_converged_reason' \
  Outputs/exodus=false \
  > outputs/ant_smoke.log 2>&1

rg -n "CONVERGED|DIVERGED|Solve Converged|Solve Did NOT Converge" outputs/ant_smoke.log || true

if rg -q "DIVERGED|Solve Did NOT Converge" outputs/ant_smoke.log; then
  echo "[smoke] transient run diverged" >&2
  exit 1
fi

echo "[smoke] PASS"

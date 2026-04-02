#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
out_root="${1:-$repo_root/outputs/debug/amr_repro}"
mkdir -p "$out_root"

stages=(
  amr_repro_stage0_bare
  amr_repro_stage1_mech_grad
  amr_repro_stage1_mech_value
  amr_repro_stage2_prodmesh_grad
  amr_repro_stage3_scalar_grad
  amr_repro_stage4_stateful_grad
  amr_repro_stage5_transport_grad
  amr_repro_stage6_offdiag_grad
  amr_repro_stage7_prodstack_bare
  amr_repro_stage7_prodstack_grad_n
)

for stage in "${stages[@]}"; do
  input="$repo_root/inputs/debug/amr_repro/${stage}.i"
  log="$out_root/${stage}.log"
  echo "=== ${stage}" | tee "$log"
  set +e
  "$repo_root/ant-opt" --allow-unused -i "$input" >>"$log" 2>&1
  code=$?
  set -e
  echo "EXIT:${code}" | tee -a "$log"
done

echo "logs: $out_root"

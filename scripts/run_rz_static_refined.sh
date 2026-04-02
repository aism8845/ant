#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
meshprep_input="$repo_root/inputs/2d/rz_static_refine_meshprep.i"
prod_input="$repo_root/inputs/2d/rz_static_run.i"

mesh_out="${1:-$repo_root/outputs/2d/rz_static_refined.e}"
outer_levels="${2:-2}"
shell_levels="${3:-1}"
right_shell_xmin="${4:-1.50}"
top_shell_ymin="${5:-0.10}"
shift $(( $# >= 5 ? 5 : $# ))

mkdir -p "$(dirname "$mesh_out")"

echo "[meshprep] output=$mesh_out outer_levels=$outer_levels shell_levels=$shell_levels right_shell_xmin=$right_shell_xmin top_shell_ymin=$top_shell_ymin"
"$repo_root/ant-opt" -i "$meshprep_input" \
  outer_refine_levels="$outer_levels" \
  shell_refine_levels="$shell_levels" \
  right_shell_xmin="$right_shell_xmin" \
  top_shell_ymin="$top_shell_ymin" \
  --mesh-only "$mesh_out"

if [[ ! -f "$mesh_out" ]]; then
  echo "Refined mesh was not created: $mesh_out" >&2
  exit 1
fi

echo "[run] mesh_file=$mesh_out"
"$repo_root/ant-opt" -i "$prod_input" mesh_file="$mesh_out" "$@"

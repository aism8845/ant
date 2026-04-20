#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

GMESH_BIN="${GMESH_BIN:-}"
if [[ -z "${GMESH_BIN}" ]]; then
  GMESH_BIN="$(command -v gmsh || true)"
fi
if [[ -z "${GMESH_BIN}" ]]; then
  GMESH_BIN="/home/amcgs/miniforge/envs/moose/bin/gmsh"
fi

cd "${SCRIPT_DIR}"

"${GMESH_BIN}" -3 square_prism_40k.geo -format msh2 -o square_prism_40k.msh
"${GMESH_BIN}" -3 right_triangle_prism_40k.geo -format msh2 -o right_triangle_prism_40k.msh

for mesh in square_prism_40k.msh right_triangle_prism_40k.msh; do
  nodes="$(awk 'f{print; exit} $0=="$Nodes"{f=1}' "${mesh}")"
  echo "${mesh}: ${nodes} nodes"
done

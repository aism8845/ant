!include rz.i

# Alternate solve path for the mixed AD/non-AD coupled Jacobian.
# Use this when the direct NEWTON path is too sensitive to the approximate
# mechanics tangent / dPK1-dn off-diagonal but you want the same physics/input stack.

[Executioner]
  solve_type := PJFNK
  petsc_options := '-snes_converged_reason -ksp_converged_reason -snes_monitor_short -snes_mf_operator'
  petsc_options_iname := '-snes_type -snes_linesearch_type -snes_linesearch_damping -ksp_type -pc_type -pc_factor_mat_solver_type -snes_rtol -snes_atol'
  petsc_options_value := 'newtonls basic 0.7 gmres lu mumps 1e-4 1e-7'
[]

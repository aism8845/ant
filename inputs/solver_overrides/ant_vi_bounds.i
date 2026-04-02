# SNES-VI lower-bound enforcement adapted from:
# - collie/cleanup/2026-02-19_collie_cleanup/inputs_solver_overrides/B_bounds_vi.i

Preconditioning/active := 'pc_smp_lu'
Bounds/active := 'n_lower_bound'

petsc_snes_type = vinewtonrsls
Executioner/solve_type := NEWTON
Executioner/line_search := basic
Executioner/petsc_options := '-snes_converged_reason -ksp_converged_reason -snes_vi_monitor'
Executioner/petsc_options_iname := '-snes_type -ksp_type -pc_type -pc_factor_mat_solver_type -snes_rtol -snes_atol'
Executioner/petsc_options_value := '${petsc_snes_type} preonly lu mumps 1e-4 1e-7'

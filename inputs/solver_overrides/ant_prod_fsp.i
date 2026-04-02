# Donor-style production solver stack adapted from:
# - collie/inputs/solver_overrides/bath48_best.i
# - yeast/biofilm/inputs/solver_overrides/bath48_best.i

Preconditioning/active := 'pc_fsp2_schur_amg'

petsc_snes_type = newtonls
Executioner/solve_type := NEWTON
Executioner/line_search := bt
Executioner/nl_abs_tol := 1e-4
Executioner/nl_rel_tol := 1e-5
Executioner/nl_max_its := 120

Executioner/petsc_options := '-snes_converged_reason -ksp_converged_reason'
Executioner/petsc_options_iname := '-snes_type -snes_linesearch_type -ksp_type -ksp_rtol -ksp_max_it -pc_type -pc_fieldsplit_type -snes_lag_preconditioner -snes_lag_preconditioner_persists -snes_lag_jacobian -snes_lag_jacobian_persists'
Executioner/petsc_options_value := '${petsc_snes_type} bt fgmres 1e-6 200 fieldsplit schur 2 true 2 true'

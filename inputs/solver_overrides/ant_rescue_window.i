# Late-window rescue override adapted from:
# - collie/cleanup/2026-02-19_collie_cleanup/inputs_solver_overrides/bath48_rescue_window.i

Preconditioning/active := 'pc_fsp2_schur_amg'

petsc_snes_type = newtonls
Executioner/scheme := implicit-euler
Executioner/nl_max_its := 160
Executioner/nl_abs_tol := 1e-4
Executioner/nl_rel_tol := 1e-5
Executioner/dtmax := 0.005
Executioner/dtmin := 1e-4

Executioner/petsc_options := '-snes_converged_reason -ksp_converged_reason'
Executioner/petsc_options_iname := '-snes_type -snes_linesearch_type -ksp_type -ksp_rtol -ksp_max_it -ksp_gmres_restart -pc_type -pc_fieldsplit_type -snes_lag_preconditioner -snes_lag_preconditioner_persists -snes_lag_jacobian -snes_lag_jacobian_persists'
Executioner/petsc_options_value := '${petsc_snes_type} bt fgmres 1e-5 300 200 fieldsplit schur 2 true 2 true'

Executioner/TimeSteppers/iter_adapt/dt := 0.0025
Executioner/TimeSteppers/iter_adapt/growth_factor := 1.0
Executioner/TimeSteppers/iter_adapt/cutback_factor := 0.5
Executioner/TimeSteppers/iter_adapt/optimal_iterations := 20
Executioner/TimeSteppers/iter_adapt/iteration_window := 4

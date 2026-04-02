# Late-window trust-region rescue override.
# Diagnostic only; keep separate from the production stack.
# Based on `ant_rescue_window.i`, but switches SNES globalization to trust region.

Preconditioning/active := 'pc_fsp2_schur_amg'

petsc_snes_type = newtontr
Executioner/scheme := implicit-euler
Executioner/nl_max_its := 160
Executioner/nl_abs_tol := 1e-4
Executioner/nl_rel_tol := 1e-5
Executioner/dtmax := 0.005
Executioner/dtmin := 1e-4

Executioner/petsc_options := '-snes_converged_reason -ksp_converged_reason'
Executioner/petsc_options_iname := '-snes_type -ksp_type -ksp_rtol -ksp_max_it -ksp_gmres_restart -pc_type -pc_fieldsplit_type -snes_lag_preconditioner -snes_lag_preconditioner_persists -snes_lag_jacobian -snes_lag_jacobian_persists'
Executioner/petsc_options_value := '${petsc_snes_type} fgmres 1e-5 300 200 fieldsplit schur 2 true 2 true'

dt := 0.0025
Executioner/TimeSteppers/iter_adapt/growth_factor := 1.0
Executioner/TimeSteppers/iter_adapt/cutback_factor := 0.5
Executioner/TimeSteppers/iter_adapt/optimal_iterations := 20
Executioner/TimeSteppers/iter_adapt/iteration_window := 4

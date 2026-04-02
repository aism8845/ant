# Strict late-window override adapted from:
# - biofilm/inputs/solver_overrides/bath48_solver_strict_window.i

Preconditioning/active := 'pc_smp_lu'

petsc_snes_type = newtonls
Executioner/solve_type := NEWTON
Executioner/line_search := bt
Executioner/nl_abs_tol := 1e-4
Executioner/nl_rel_tol := 1e-5
Executioner/nl_max_its := 120
Executioner/dtmax := 0.02

Executioner/petsc_options := '-snes_converged_reason -ksp_converged_reason'
Executioner/petsc_options_iname := '-snes_type -snes_linesearch_type -ksp_type -pc_type -pc_factor_mat_solver_type -snes_rtol -snes_atol'
Executioner/petsc_options_value := '${petsc_snes_type} bt preonly lu mumps 1e-4 1e-7'

Executioner/TimeSteppers/iter_adapt/optimal_iterations := 12
Executioner/TimeSteppers/iter_adapt/iteration_window := 2
Executioner/TimeSteppers/iter_adapt/growth_factor := 1.03
Executioner/TimeSteppers/iter_adapt/cutback_factor := 0.5

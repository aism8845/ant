# Strict late-window overlay that preserves the donor FSP/Schur base stack.
# Use this on top of `ant_prod_fsp.i`.

petsc_snes_type = newtonls
Executioner/solve_type := NEWTON
Executioner/line_search := bt
Executioner/nl_abs_tol := 1e-4
Executioner/nl_rel_tol := 1e-5
Executioner/nl_max_its := 120
Executioner/dtmax := 0.02

Executioner/TimeSteppers/iter_adapt/optimal_iterations := 12
Executioner/TimeSteppers/iter_adapt/iteration_window := 2
Executioner/TimeSteppers/iter_adapt/growth_factor := 1.03
Executioner/TimeSteppers/iter_adapt/cutback_factor := 0.5

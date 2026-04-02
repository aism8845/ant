Solver notes
============

This file consolidates the useful solver guidance from the current `CellGel`, `buckling`, and `biofilm` app handoffs.

Recommended default for this app
--------------------------------
For the coupled mixture+n transport path in `ant`:

- `Transient`
- `scheme = bdf2`
- `solve_type = NEWTON`
- `line_search = bt`
- `automatic_scaling = true`
- direct linear solve for harder runs:
  - PETSc: `-snes_type newtonls -ksp_type preonly -pc_type lu -pc_factor_mat_solver_type mumps`
- adaptive stepping for larger production runs:
  - `dt = 0.02`
  - `dtmin = 1e-3`
  - `growth_factor = 1.05`
  - `cutback_factor = 0.5`
  - `optimal_iterations = 6`
- practical move-on rules for sweeps:
  - stop at `dt <= 1e-3`
  - use a conservative wallclock cutoff rather than letting one case block a family

These settings come from the recent CellGel RVE production sweeps and were the most stable “balanced” stack for the large-deformation soft corner.

Buckling / biofilm presets worth keeping
----------------------------------------

1. Max speed
- Source: `buckling/README.md`
- Key idea:
  - SMP preconditioner + LU/MUMPS
  - `automatic_scaling = true`
- Use when the problem is moderate and you need throughput over strict trajectory fidelity.

2. Speed + accuracy
- Source: `buckling/README.md`
- This is the recommended default family in those notes.
- Use a tighter lockstep / production stack rather than the loosest speed-only settings.

3. Max accuracy / reference
- Source: `buckling/README.md`
- Use the slowest lockstep/reference settings only when benchmarking trajectories.

Important negative guidance
---------------------------
- Do not rely on PETSc arc-length (`SNESNEWTONAL`) unless callback wiring is explicitly present and verified.
- Do not add Riks/continuation machinery to this app by default.
- Treat difficult high-growth branches as a model/timestep/history problem before escalating solver complexity.

From biofilm handoffs
---------------------
- Keep mechanics on the non-AD Total Lagrangian stress path with an explicit consistent tangent.
- Keep nutrient PDE fully AD.
- Use explicit mechanics←nutrient off-diagonal via `d(pk1_stress)/dn`.
- Evaluate `d(pk1_stress)/dn` by centered finite difference through the same constitutive replay helper.
- Clamp nutrient probes in the finite-difference stencil:
  - `n_plus = max(n + dn, 0)`
  - `n_minus = max(n - dn, 0)`
  - divide by `max(n_plus - n_minus, 1e-12)`

Jacobian test target
--------------------
- Target agreement: `~1e-7`
- Acceptable smoke threshold in this app: `<= 1e-6`
- Check using:
  - `-snes_test_jacobian`

Files that motivated this summary
---------------------------------
- `buckling/README.md`
- `biofilm/doc/CURRENT_REPO_STATE_2026-02-19.md`
- `biofilm/notes/CALIBRATION_RULES_AND_LEARNINGS_2026-02-26.md`
- recent CellGel coupled-sweep handoff settings

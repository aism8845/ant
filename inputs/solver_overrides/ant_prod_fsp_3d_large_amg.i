# Fast/accurate 3D solver stack for larger refined puck meshes.
# Keeps full Newton and fieldsplit, but replaces the mechanics LU subsolve
# with elasticity-oriented BoomerAMG to avoid MUMPS scaling issues.

Preconditioning/active := 'pc_fsp3_mult_amg'

petsc_snes_type = newtonls
Executioner/solve_type := NEWTON
Executioner/line_search := bt
Executioner/nl_abs_tol := 1e-4
Executioner/nl_rel_tol := 1e-5
Executioner/nl_max_its := 100

Executioner/petsc_options := '-snes_converged_reason -ksp_converged_reason -snes_ksp_ew'
Executioner/petsc_options_iname := '-snes_type -snes_linesearch_type -ksp_type -ksp_rtol -ksp_max_it -ksp_gmres_restart -pc_type -pc_fieldsplit_type -snes_lag_preconditioner -snes_lag_preconditioner_persists -snes_lag_jacobian -snes_lag_jacobian_persists'
Executioner/petsc_options_value := '${petsc_snes_type} bt fgmres 1e-4 150 150 fieldsplit multiplicative 2 true 2 true'

[Preconditioning]
  active = 'pc_fsp3_mult_amg'

  [pc_fsp3_mult_amg]
    type = FSP
    topsplit = 'mech_nutr'

    [mech_nutr]
      splitting = 'mech nutr'
      splitting_type = multiplicative
    []

    [mech]
      vars = 'ux uy uz'
      petsc_options_iname = '-ksp_type -pc_type -pc_hypre_type -pc_hypre_boomeramg_strong_threshold -pc_hypre_boomeramg_interp_type -pc_hypre_boomeramg_coarsen_type -pc_hypre_boomeramg_agg_nl -pc_hypre_boomeramg_nodal_coarsen -pc_hypre_boomeramg_vec_interp_variant -pc_hypre_boomeramg_max_levels'
      petsc_options_value = 'preonly hypre boomeramg 0.7 ext+i HMIS 3 4 2 15'
    []

    [nutr]
      vars = 'n'
      petsc_options_iname = '-ksp_type -pc_type -pc_hypre_type -pc_hypre_boomeramg_strong_threshold -pc_hypre_boomeramg_agg_nl'
      petsc_options_value = 'preonly hypre boomeramg 0.7 2'
    []
  []
[]

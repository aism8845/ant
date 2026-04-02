# Build the base mesh and speckle field first from the repo root:
#   gmsh inputs/2d/rz.geo -2 -format msh2 -o inputs/2d/rz.msh
#   ./ant-opt -i inputs/2d/phi.i
#
# Optional static refinement workflow:
#   ./ant-opt -i inputs/2d/rz_static_refine_meshprep.i --mesh-only outputs/2d/rz_static_refined.e
#   ./ant-opt -i inputs/2d/rz_static_run.i mesh_file=/abs/path/to/rz_static_refined.e

mesh_file = rz.msh
phi_file = ../../outputs/2d/phi.e

end_time = 10
dt = 0.02
n_supply = 1.0
front_dt_cap = 0.05
front_dt_cap_start = 3.0
front_dt_cap_end = 8.5
front_dt_cap_hi = 1e6
petsc_snes_type = newtonls

G_cell = 1.0
K_cell = 10.0
ke0 = 0.24
p_star = 1.0
k_t = 0.0

G_gel = 0.10
Im = 100.0
kD0 = 0.02
kDinf = 0.08
sig_yield = 1.0
n_kD = 2.0

phi0 = 0.10
k_ap = 0.0
epsilon = 1e-7
smooth_eps_n = 1e-8

nut_str = 0.005
m_nut = 3
D_nutrient = 10
D_floor = 0.001
gamma_n0 = 750
phi_max = 0.9
gate_phi_on_ke = false
phi_gate_start = 0.70
phi_gate_end = 0.88
viscous_reg_coef = 0.0
follower_pressure_factor = 0.0
crowd_exp = 2.0
smooth_eps_c = 1e-6
smooth_eps_D = 1e-12
n_min_guard = 0.0
surface_profile_boundary = top
surface_profile_file_base = outputs/2d/rz_surface_profile
surface_profile_interval = 1.0

[GlobalParams]
  displacements = 'ux uy'
[]

[Mesh]
  coord_type = RZ
  rz_coord_axis = Y
  parallel_type = distributed
  [base]
    type = FileMeshGenerator
    file = ${mesh_file}
  []
  final_generator = base
[]

[Variables]
  [n]
    family = LAGRANGE
    order = FIRST
  []
[]

[AuxVariables]
  [phi_ref_ic]
    family = LAGRANGE
    order = FIRST
  []
  [bounds_dummy]
    family = LAGRANGE
    order = FIRST
  []
  [phi_cell_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [eta_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [kT1_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [k_diss_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [ke_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [kh_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [fa_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [gp_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [g_phi_ke_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [gate_total_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [press_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [J_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [J_nutr_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [Jdot_nutr_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [n_sink_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [D_phys_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [phi_cell_nutr_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [J_mech_nutr_aux]
    family = MONOMIAL
    order = CONSTANT
  []
  [F_inv_guard_aux]
    family = MONOMIAL
    order = CONSTANT
  []
[]

[UserObjects]
  [phi_ref_solution]
    type = SolutionUserObject
    mesh = ${phi_file}
    system_variables = 'phi_ref_ic'
    timestep = LATEST
  []
[]

[ICs]
  active = 'phi_ref_from_file n0'

  [phi_ref_from_file]
    type = SolutionIC
    variable = phi_ref_ic
    solution_uo = phi_ref_solution
    from_variable = phi_ref_ic
  []

  [n0]
    type = ConstantIC
    variable = n
    value = 0.5
  []
[]

[Functions]
  [front_dt_cap_fn]
    type = ParsedFunction
    expression = 'if(t < t0, dthi, if(t <= t1, dtcap, dthi))'
    symbol_names = 't0 t1 dtcap dthi'
    symbol_values = '${front_dt_cap_start} ${front_dt_cap_end} ${front_dt_cap} ${front_dt_cap_hi}'
  []
[]

[Physics]
  [SolidMechanics]
    [QuasiStatic]
      new_system = true
      add_variables = true
      displacements = 'ux uy'
      strain = FINITE
      formulation = TOTAL
      use_automatic_differentiation = false
      [all] []
    []
  []
[]

[Kernels]
  [u_n_offdiag_x]
    type = TLStressDivergenceNutrientOffDiagRZ
    variable = ux
    component = 0
    displacements = 'ux uy'
    large_kinematics = true
    n = n
  []
  [u_n_offdiag_y]
    type = TLStressDivergenceNutrientOffDiagRZ
    variable = uy
    component = 1
    displacements = 'ux uy'
    large_kinematics = true
    n = n
  []
  [viscous_reg_x]
    type = CoefTimeDerivative
    variable = ux
    Coefficient = ${viscous_reg_coef}
  []
  [viscous_reg_y]
    type = CoefTimeDerivative
    variable = uy
    Coefficient = ${viscous_reg_coef}
  []
  [n_td]
    type = ADJnTimeDerivative
    variable = n
    coef = J_nutr
    coef_dot = Jdot_nutr
    include_jdot = true
  []
  [n_diff]
    type = ADMatTensorDiffusion
    variable = n
    diffusivity = D_eff_nutr
  []
  [n_rxn]
    type = ADMatReactionSigned
    variable = n
    reaction_rate = n_source_ref_nutr
  []
[]

[BCs]
  [ux_axis]
    type = DirichletBC
    variable = ux
    boundary = left
    value = 0.0
  []
  [uy_bottom]
    type = DirichletBC
    variable = uy
    boundary = bottom
    value = 0.0
  []
  [n_sym_axis]
    type = NeumannBC
    variable = n
    boundary = left
    value = 0.0
  []
  [n_sym_bottom]
    type = NeumannBC
    variable = n
    boundary = bottom
    value = 0.0
  []
  [n_supply]
    type = DirichletBC
    variable = n
    boundary = 'top right'
    value = ${n_supply}
  []
  [top_follower_pressure]
    type = Pressure
    variable = uy
    boundary = ${surface_profile_boundary}
    factor = ${follower_pressure_factor}
  []
[]

[Bounds]
  active = ''
  [n_lower_bound]
    type = ConstantBounds
    variable = bounds_dummy
    bounded_variable = n
    bound_type = lower
    bound_value = ${n_min_guard}
  []
[]

[Materials]
  [mix]
    type = CellGelMixtureNut2
    G_cell = ${G_cell}
    K_cell = ${K_cell}
    ke0 = ${ke0}
    p_star = ${p_star}
    k_t = ${k_t}
    G_gel = ${G_gel}
    Im = ${Im}
    kD0 = ${kD0}
    kDinf = ${kDinf}
    sig_yield = ${sig_yield}
    n_kD = ${n_kD}
    phi_cell_0 = ${phi0}
    k_ap = ${k_ap}
    epsilon = ${epsilon}
    nut_str = ${nut_str}
    m_nut = ${m_nut}
    smooth_eps_n = ${smooth_eps_n}
    phi_max = ${phi_max}
    gate_gp_on_ke = true
    gate_fa_on_ke = true
    gate_phi_on_ke = ${gate_phi_on_ke}
    phi_gate_start = ${phi_gate_start}
    phi_gate_end = ${phi_gate_end}
    n = n
    phi_ref_ic = phi_ref_ic
  []
  [nut]
    type = ADNutrientTLTransportRZ
    n = n
    disp_r = ux
    disp_z = uy
    axisymmetric = true
    radial_coord = 0
    r_eps = 1e-12
    phi_cell_ref = phi_cell_ref
    D_nutrient = ${D_nutrient}
    D_floor = ${D_floor}
    gamma_n0 = ${gamma_n0}
    nut_str = ${nut_str}
    m_nut = ${m_nut}
    use_crowding_diffusion = true
    phi_max = ${phi_max}
    crowd_exp = ${crowd_exp}
    smooth_eps_c = ${smooth_eps_c}
    smooth_eps_D = ${smooth_eps_D}
    safe_F_inv = true
    J_inv_floor = 1e-10
  []
[]

[AuxKernels]
  [phi_cell_out]
    type = MaterialRealAux
    variable = phi_cell_aux
    property = phi_cell
    execute_on = 'initial timestep_end'
  []
  [eta_out]
    type = MaterialRealAux
    variable = eta_aux
    property = eta
    execute_on = 'initial timestep_end'
  []
  [kT1_out]
    type = MaterialRealAux
    variable = kT1_aux
    property = kT1
    execute_on = 'initial timestep_end'
  []
  [k_diss_out]
    type = MaterialRealAux
    variable = k_diss_aux
    property = k_diss
    execute_on = 'initial timestep_end'
  []
  [ke_out]
    type = MaterialRealAux
    variable = ke_aux
    property = ke
    execute_on = 'initial timestep_end'
  []
  [kh_out]
    type = MaterialRealAux
    variable = kh_aux
    property = kh
    execute_on = 'initial timestep_end'
  []
  [fa_out]
    type = MaterialRealAux
    variable = fa_aux
    property = fa
    execute_on = 'initial timestep_end'
  []
  [gp_out]
    type = MaterialRealAux
    variable = gp_aux
    property = gp
    execute_on = 'initial timestep_end'
  []
  [g_phi_ke_out]
    type = MaterialRealAux
    variable = g_phi_ke_aux
    property = g_phi_ke
    execute_on = 'initial timestep_end'
  []
  [gate_total_out]
    type = MaterialRealAux
    variable = gate_total_aux
    property = gate_total
    execute_on = 'initial timestep_end'
  []
  [press_out]
    type = MaterialRealAux
    variable = press_aux
    property = pressure
    execute_on = 'initial timestep_end'
  []
  [J_out]
    type = MaterialRealAux
    variable = J_aux
    property = J_mix
    execute_on = 'initial timestep_end'
  []
  [J_nutr_out]
    type = MaterialRealAux
    variable = J_nutr_aux
    property = J_nutr_pp
    execute_on = 'initial timestep_end'
  []
  [Jdot_nutr_out]
    type = MaterialRealAux
    variable = Jdot_nutr_aux
    property = Jdot_nutr_pp
    execute_on = 'initial timestep_end'
  []
  [n_sink_out]
    type = MaterialRealAux
    variable = n_sink_aux
    property = n_source_ref_nutr_pp
    execute_on = 'initial timestep_end'
  []
  [D_phys_out]
    type = MaterialRealAux
    variable = D_phys_aux
    property = D_phys_nutr
    execute_on = 'initial timestep_end'
  []
  [phi_cell_nutr_out]
    type = MaterialRealAux
    variable = phi_cell_nutr_aux
    property = phi_cell_nutr
    execute_on = 'initial timestep_end'
  []
  [J_mech_nutr_out]
    type = MaterialRealAux
    variable = J_mech_nutr_aux
    property = J_mech_nutr
    execute_on = 'initial timestep_end'
  []
  [F_inv_guard_out]
    type = MaterialRealAux
    variable = F_inv_guard_aux
    property = F_inv_guard_flag_nutr
    execute_on = 'initial timestep_end'
  []
[]

[Postprocessors]
  [dt]
    type = TimestepSize
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [nonlinear_its]
    type = NumNonlinearIterations
    execute_on = 'timestep_end'
    outputs = 'solver_watch'
  []
  [linear_its]
    type = NumLinearIterations
    execute_on = 'timestep_end'
    outputs = 'solver_watch'
  []

  [avg_n]
    type = ElementAverageValue
    variable = n
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [n_min]
    type = NodalExtremeValue
    variable = n
    value_type = min
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [n_max]
    type = NodalExtremeValue
    variable = n
    value_type = max
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []

  [avg_phi_cell]
    type = ElementAverageValue
    variable = phi_cell_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [min_phi_cell]
    type = ElementExtremeValue
    variable = phi_cell_aux
    value_type = min
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [max_phi_cell]
    type = ElementExtremeValue
    variable = phi_cell_aux
    value_type = max
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []

  [avg_phi_cell_nutr]
    type = ElementAverageValue
    variable = phi_cell_nutr_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_ke]
    type = ElementAverageValue
    variable = ke_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [max_ke]
    type = ElementExtremeValue
    variable = ke_aux
    value_type = max
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_kh]
    type = ElementAverageValue
    variable = kh_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [max_kh]
    type = ElementExtremeValue
    variable = kh_aux
    value_type = max
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_kT1]
    type = ElementAverageValue
    variable = kT1_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_k_diss]
    type = ElementAverageValue
    variable = k_diss_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_eta]
    type = ElementAverageValue
    variable = eta_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_fa]
    type = ElementAverageValue
    variable = fa_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [min_fa]
    type = ElementExtremeValue
    variable = fa_aux
    value_type = min
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_gp]
    type = ElementAverageValue
    variable = gp_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [min_gp]
    type = ElementExtremeValue
    variable = gp_aux
    value_type = min
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_press]
    type = ElementAverageValue
    variable = press_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [min_press]
    type = ElementExtremeValue
    variable = press_aux
    value_type = min
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [max_press]
    type = ElementExtremeValue
    variable = press_aux
    value_type = max
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_J]
    type = ElementAverageValue
    variable = J_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [min_J]
    type = ElementExtremeValue
    variable = J_aux
    value_type = min
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_J_nutr]
    type = ElementAverageValue
    variable = J_nutr_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_Jdot_nutr]
    type = ElementAverageValue
    variable = Jdot_nutr_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_D_phys]
    type = ElementAverageValue
    variable = D_phys_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [min_D_phys]
    type = ElementExtremeValue
    variable = D_phys_aux
    value_type = min
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_n_sink]
    type = ElementAverageValue
    variable = n_sink_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [avg_J_mech_nutr]
    type = ElementAverageValue
    variable = J_mech_nutr_aux
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
  [max_F_inv_guard]
    type = ElementExtremeValue
    variable = F_inv_guard_aux
    value_type = max
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
[]

[VectorPostprocessors]
  [surface_top]
    type = SideValueSampler
    boundary = ${surface_profile_boundary}
    variable = 'ux uy'
    sort_by = x
    use_displaced_mesh = true
    execute_on = 'initial timestep_end final'
    outputs = 'surface_profile_csv'
  []
[]


[Debug]
  show_var_residual_norms = true
[]

[Executioner]
  type = Transient
  scheme = bdf2
  solve_type = NEWTON
  line_search = basic
  end_time = ${end_time}
  dtmin = 1e-3
  nl_rel_tol = 1e-5
  nl_abs_tol = 1e-8
  nl_max_its = 80

  automatic_scaling = true
  compute_scaling_once = false
  off_diagonals_in_auto_scaling = true

  petsc_options = '-snes_converged_reason -ksp_converged_reason -snes_monitor_short'
  petsc_options_iname = '-snes_type -snes_linesearch_type -snes_linesearch_damping -ksp_type -pc_type -pc_factor_mat_solver_type -snes_rtol -snes_atol'
  petsc_options_value = '${petsc_snes_type} basic 0.7 preonly lu mumps 1e-4 1e-7'

  [TimeSteppers]
    [iter_adapt]
      type = IterationAdaptiveDT
      dt = ${dt}
      growth_factor = 1.1
      cutback_factor = 0.5
      optimal_iterations = 8
    []
    [front_cap]
      type = FunctionDT
      function = front_dt_cap_fn
      min_dt = 1e-3
    []
  []
[]

[Preconditioning]
  active = 'pc_smp_lu'

  [pc_smp_lu]
    type = SMP
    full = true
  []

  [pc_fsp2_schur_amg]
    type = FSP
    topsplit = 'mech_nutr'

    [mech_nutr]
      splitting = 'mech nutr'
      splitting_type = schur
      petsc_options_iname = '-pc_fieldsplit_schur_fact_type -pc_fieldsplit_schur_precondition'
      petsc_options_value = 'full selfp'
    []

    [mech]
      vars = 'ux uy'
      petsc_options_iname = '-ksp_type -pc_type -pc_factor_mat_solver_type'
      petsc_options_value = 'preonly lu mumps'
    []

    [nutr]
      vars = 'n'
      petsc_options_iname = '-ksp_type -pc_type -pc_hypre_type'
      petsc_options_value = 'preonly hypre boomeramg'
    []
  []

  [pc_fsp2_schur_gamg]
    type = FSP
    topsplit = 'mech_nutr'

    [mech_nutr]
      splitting = 'mech nutr'
      splitting_type = schur
      petsc_options_iname = '-pc_fieldsplit_schur_fact_type -pc_fieldsplit_schur_precondition'
      petsc_options_value = 'full selfp'
    []

    [mech]
      vars = 'ux uy'
      petsc_options_iname = '-ksp_type -pc_type -pc_factor_mat_solver_type'
      petsc_options_value = 'preonly lu mumps'
    []

    [nutr]
      vars = 'n'
      petsc_options_iname = '-ksp_type -pc_type'
      petsc_options_value = 'preonly gamg'
    []
  []
[]

[Outputs]
  perf_graph = false
  print_linear_residuals = true
  [console]
    type = Console
    max_rows = 1
    outlier_variable_norms = false
    execute_postprocessors_on = none
  []
  [exodus]
    type = Exodus
    file_base = outputs/2d/rz
    execute_on = 'initial timestep_end final'
    show = 'ux uy n phi_ref_ic phi_cell_aux eta_aux kT1_aux k_diss_aux ke_aux kh_aux fa_aux gp_aux g_phi_ke_aux gate_total_aux press_aux J_aux J_nutr_aux Jdot_nutr_aux n_sink_aux D_phys_aux phi_cell_nutr_aux J_mech_nutr_aux F_inv_guard_aux'
  []
  [csv]
    type = CSV
    file_base = outputs/2d/rz
    execute_on = 'initial timestep_end final'
  []
  [solver_watch]
    type = CSV
    file_base = outputs/2d/rz_solver_watch
    execute_on = 'initial timestep_end'
    show = 'dt nonlinear_its linear_its avg_n n_min n_max avg_phi_cell min_phi_cell max_phi_cell avg_phi_cell_nutr avg_ke max_ke avg_kh max_kh avg_kT1 avg_k_diss avg_eta avg_fa min_fa avg_gp min_gp avg_press min_press max_press avg_J min_J avg_J_nutr avg_Jdot_nutr avg_D_phys min_D_phys avg_n_sink avg_J_mech_nutr max_F_inv_guard'
  []
  [surface_profile_csv]
    type = CSV
    file_base = ${surface_profile_file_base}
    execute_on = none
    execute_vector_postprocessors_on = 'initial timestep_end final'
    min_simulation_time_interval = ${surface_profile_interval}
    show = 'surface_top'
    time_data = true
  []
  [chk]
    type = Checkpoint
    file_base = outputs/2d/rz_chk
    num_files = 32
    time_step_interval = 1
    min_simulation_time_interval = 1.0
  []
[]

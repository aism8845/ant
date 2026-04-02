mesh_file = ../../2d/rz.msh
end_time = 0.02
dt = 0.02
u_right = 1e-3
phi0 = 0.08

[Mesh]
  coord_type = RZ
  rz_coord_axis = Y
  parallel_type = replicated
  file = ${mesh_file}
[]

[GlobalParams]
  displacements = 'ux uy'
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
[]

[ICs]
  [n0]
    type = ConstantIC
    variable = n
    value = 0.0
  []
[]

[AuxKernels]
  [phi_ref_const]
    type = ConstantAux
    variable = phi_ref_ic
    value = ${phi0}
    execute_on = initial
  []
[]

[Physics/SolidMechanics/QuasiStatic]
  new_system = true
  [all]
    add_variables = true
    strain = FINITE
    formulation = TOTAL
  []
[]

[Kernels]
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
  [ux_right]
    type = DirichletBC
    variable = ux
    boundary = right
    value = ${u_right}
  []
  [n_supply]
    type = DirichletBC
    variable = n
    boundary = 'top right'
    value = 1.0
  []
[]

[Materials]
  [mix]
    type = CellGelMixtureNut2
    G_cell = 1.0
    K_cell = 2.6666666667
    ke0 = 0.0
    k_exp_max = 0.0
    p_star = 1.0
    k_t = 0.0
    G_gel = 0.5
    Im = 100.0
    kD0 = 0.0
    kDinf = 0.0
    sig_yield = 1.0
    n_kD = 2.0
    phi_cell_0 = ${phi0}
    k_ap = 0.0
    epsilon = 1e-7
    nut_str = 0.2
    m_nut = 2.0
    smooth_eps_n = 1e-8
    gate_gp_on_ke = true
    gate_fa_on_ke = true
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
    D_nutrient = 0.1
    D_floor = 1e-4
    gamma_n0 = 0.5
    nut_str = 0.2
    m_nut = 2.0
    use_crowding_diffusion = true
    phi_max = 0.95
    crowd_exp = 2.0
    smooth_eps_c = 1e-8
    smooth_eps_D = 1e-8
    safe_F_inv = true
    J_inv_floor = 1e-10
  []
[]

[Adaptivity]
  max_h_level = 1
  interval = 1
  initial_steps = 0
  cycles_per_step = 0
  [Indicators]
    [probe]
      type = GradientJumpIndicator
      variable = n
      use_displaced_mesh = false
    []
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
  automatic_scaling = true
  compute_scaling_once = false
  nl_rel_tol = 1e-6
  nl_abs_tol = 1e-8
  l_tol = 1e-12
  l_max_its = 50
  end_time = ${end_time}

  petsc_options_iname = '-pc_type'
  petsc_options_value = 'lu'

  [TimeStepper]
    type = ConstantDT
    dt = ${dt}
  []
[]

[Outputs]
  exodus = false
  csv = false
[]

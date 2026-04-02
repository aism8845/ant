mesh_file = ../../2d/rz.msh
phi_file = ../../../outputs/2d/phi.e

end_time = 0.02
dt = 0.02
n_supply = 1.0

G_cell = 1.0
K_cell = 10.0
ke0 = 0.08
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

nut_str = 0.20
m_nut = 2.0
D_nutrient = 0.1
D_floor = 1e-10
gamma_n0 = 0.5
phi_max = 0.95
crowd_exp = 2.0
smooth_eps_c = 1e-6
smooth_eps_D = 1e-12

[GlobalParams]
  displacements = 'ux uy'
[]

[Mesh]
  coord_type = RZ
  rz_coord_axis = Y
  parallel_type = distributed
  file = ${mesh_file}
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

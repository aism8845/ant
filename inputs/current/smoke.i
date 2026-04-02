[GlobalParams]
  displacements = 'ux uy'
[]

[Mesh]
  [rect]
    type = GeneratedMeshGenerator
    dim = 2
    nx = 16
    ny = 16
    xmin = 0.0
    xmax = 1.0
    ymin = 0.0
    ymax = 1.0
  []
[]

[Variables]
  [n]
    family = LAGRANGE
    order = FIRST
  []
[]

[AuxVariables]
  [phi_cell_aux]
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
  [press_aux]
    family = MONOMIAL
    order = CONSTANT
  []
[]

[ICs]
  [n0]
    type = ConstantIC
    variable = n
    value = 0.2
  []
[]

[Physics]
  [SolidMechanics]
    [QuasiStatic]
      new_system = true
      [all]
        add_variables = true
        displacements = 'ux uy'
        strain = FINITE
        formulation = TOTAL
      []
    []
  []
[]

[Kernels]
  [u_n_offdiag_x]
    type = TLStressDivergenceNutrientOffDiag
    variable = ux
    component = 0
    displacements = 'ux uy'
    large_kinematics = true
    n = n
  []
  [u_n_offdiag_y]
    type = TLStressDivergenceNutrientOffDiag
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
  [ux_left]
    type = DirichletBC
    variable = ux
    boundary = left
    value = 0.0
  []
  [ux_right]
    type = DirichletBC
    variable = ux
    boundary = right
    value = 0.0
  []
  [uy_bottom]
    type = DirichletBC
    variable = uy
    boundary = bottom
    value = 0.0
  []
  [n_top]
    type = DirichletBC
    variable = n
    boundary = top
    value = 1.0
  []
[]

[Materials]
  [mix]
    type = CellGelMixtureNut2
    G_cell = 1.0
    K_cell = 2.6666666667
    ke0 = 0.08
    t_str = 0.1
    m_exp = 4.0
    p_star = 1.0
    press_gate_smooth = 0.02
    k_t = 0.1
    G_gel = 0.5
    Im = 100.0
    kD0 = 0.02
    kDinf = 0.08
    sig_yield = 1.0
    n_kD = 2.0
    phi_cell_0 = 0.1
    k_ap = 0.0
    epsilon = 1e-7
    nut_str = 0.2
    m_nut = 2.0
    gate_gp_on_ke = true
    gate_fa_on_ke = true
    n = n
  []
  [nut]
    type = ADNutrientTLTransport
    n = n
    disp_x = ux
    disp_y = uy
    phi_cell_ref = phi_cell_ref
    D_nutrient = 0.1
    D_floor = 1e-10
    gamma_n0 = 0.5
    nut_str = 0.2
    m_nut = 2.0
    use_crowding_diffusion = true
    phi_max = 0.95
    crowd_exp = 2.0
  []
[]

[AuxKernels]
  [phi_cell_out]
    type = MaterialRealAux
    variable = phi_cell_aux
    property = phi_cell
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
  [press_out]
    type = MaterialRealAux
    variable = press_aux
    property = pressure
    execute_on = 'initial timestep_end'
  []
[]

[Executioner]
  type = Transient
  scheme = bdf2
  solve_type = NEWTON
  line_search = bt
  automatic_scaling = true
  dtmin = 1e-6
  dtmax = 2e-3
  end_time = 0.02
  nl_rel_tol = 1e-8
  nl_abs_tol = 1e-10
  nl_max_its = 25
  l_tol = 1e-10
  l_max_its = 100

  [TimeStepper]
    type = ConstantDT
    dt = 2e-3
  []
[]

[Preconditioning]
  [pc]
    type = SMP
    full = true
  []
[]

[Outputs]
  exodus = false
  csv = true
[]

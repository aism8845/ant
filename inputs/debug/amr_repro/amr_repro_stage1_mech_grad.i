end_time = 0.02
dt = 0.02
u_right = 1e-3
E = 1.0
nu = 0.3

[Mesh]
  coord_type = RZ
  rz_coord_axis = Y
  parallel_type = replicated
  [gen]
    type = GeneratedMeshGenerator
    dim = 2
    nx = 24
    ny = 6
    xmin = 0.0
    xmax = 2.0
    ymin = 0.0
    ymax = 0.5
  []
[]

[GlobalParams]
  displacements = 'ux uy'
[]

[Physics/SolidMechanics/QuasiStatic]
  new_system = true
  [all]
    add_variables = true
    strain = FINITE
    formulation = TOTAL
  []
[]

[Materials]
  [elasticity]
    type = ComputeIsotropicElasticityTensor
    youngs_modulus = ${E}
    poissons_ratio = ${nu}
  []
  [stress]
    type = ComputeLagrangianLinearElasticStress
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
[]

[Adaptivity]
  max_h_level = 1
  interval = 1
  initial_steps = 0
  cycles_per_step = 0
  [Indicators]
    [probe]
      type = GradientJumpIndicator
      variable = ux
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

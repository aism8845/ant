# 3D speckle field on the symmetric refined puck mesh:
#   phi0 = 0.08
#   Aphi = 0.015
#   lcorr = 0.015

mesh_file = puck_refined_symmetric.msh
phi0 = 0.08
Aphi = 0.015
lcorr = 0.015
seed = 12345

phi_min = ${fparse phi0 - Aphi}
phi_max = ${fparse phi0 + Aphi}

Dflt = 1.0
t_end = ${fparse lcorr * lcorr / Dflt}

[Mesh]
  parallel_type = distributed
  file = ${mesh_file}
[]

[Variables]
  [phi_ref_ic]
    family = LAGRANGE
    order = FIRST
  []
[]

[ICs]
  [phi_ref_rand]
    type = RandomIC
    variable = phi_ref_ic
    min = ${phi_min}
    max = ${phi_max}
    seed = ${seed}
  []
[]

[Kernels]
  [td]
    type = TimeDerivative
    variable = phi_ref_ic
  []
  [diff]
    type = Diffusion
    variable = phi_ref_ic
  []
[]

[Dampers]
  [bounds]
    type = BoundingValueNodalDamper
    variable = phi_ref_ic
    min_value = 1e-8
    max_value = 0.9999
  []
[]

[Executioner]
  type = Transient
  scheme = bdf2
  solve_type = LINEAR

  dt = ${fparse t_end / 20}
  dtmin = ${fparse t_end / 200}
  dtmax = ${fparse t_end / 10}
  end_time = ${t_end}

  petsc_options_iname = '-ksp_type -pc_type -ksp_rtol'
  petsc_options_value = 'cg hypre 1e-12'
[]

[Outputs]
  [out]
    type = Exodus
    file_base = outputs/3d/phi_match2d_caseA_aphi0015_refined_symmetric
    time_step_interval = 1
  []
[]

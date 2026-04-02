# Build the mesh first from the repo root:
#   gmsh inputs/2d/rz.geo -2 -format msh2 -o inputs/2d/rz.msh

mesh_file = rz.msh
phi0 = 0.10
Aphi = 0.04
lcorr = 0.03
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
    file_base = outputs/2d/phi
    time_step_interval = 1
  []
[]

# Optional adaptive displacement-rate regularization.
# Uses the previous step's nonlinear iteration count as a distress signal.
# This is intentionally separate from the production stack.

viscous_reg_base = 0.0
viscous_reg_nl_target = 1.0
viscous_reg_nl_gain = 0.002
viscous_reg_min = 0.0
viscous_reg_max = 0.02

viscous_reg_coef = ${viscous_reg_base}

[ChainControls]
  [get_nonlinear_its_for_visc]
    type = GetPostprocessorChainControl
    postprocessor = nonlinear_its
    execute_on = 'initial timestep_begin'
  []
  [viscous_reg_raw]
    type = ParsedChainControl
    expression = 'base + gain * max(nlits - target, 0)'
    symbol_names = 'base gain nlits target'
    symbol_values = '${viscous_reg_base} ${viscous_reg_nl_gain} get_nonlinear_its_for_visc:value ${viscous_reg_nl_target}'
    execute_on = 'initial timestep_begin'
  []
  [viscous_reg_limited]
    type = LimitChainControl
    control_data = viscous_reg_raw:value
    min_value = ${viscous_reg_min}
    max_value = ${viscous_reg_max}
    execute_on = 'initial timestep_begin'
  []
  [set_viscous_reg_x]
    type = SetRealValueChainControl
    parameter = Kernels/viscous_reg_x/Coefficient
    value = viscous_reg_limited:value
    execute_on = 'initial timestep_begin'
  []
  [set_viscous_reg_y]
    type = SetRealValueChainControl
    parameter = Kernels/viscous_reg_y/Coefficient
    value = viscous_reg_limited:value
    execute_on = 'initial timestep_begin'
  []
[]

[Postprocessors]
  [viscous_reg_effective]
    type = ChainControlDataPostprocessor
    chain_control_data_name = viscous_reg_limited:value
    execute_on = 'initial timestep_end'
    outputs = 'solver_watch'
  []
[]

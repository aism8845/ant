# 3D MATLAB-oriented profile and analysis outputs.
# - 10 diameters across the top surface
# - profile output about every 2 simulation seconds
# - separate analysis CSV every timestep_end

surface_profile_interval = 2.0
surface_profile_num_points = 401
surface_profile_z = 0.999

[VectorPostprocessors]
  [surface_line_00]
    type = LineValueSampler
    start_point = '-1.98 0.0 ${surface_profile_z}'
    end_point = '1.98 0.0 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
  [surface_line_01]
    type = LineValueSampler
    start_point = '-1.883092 -0.611854 ${surface_profile_z}'
    end_point = '1.883092 0.611854 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
  [surface_line_02]
    type = LineValueSampler
    start_point = '-1.601854 -1.163815 ${surface_profile_z}'
    end_point = '1.601854 1.163815 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
  [surface_line_03]
    type = LineValueSampler
    start_point = '-1.163815 -1.601854 ${surface_profile_z}'
    end_point = '1.163815 1.601854 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
  [surface_line_04]
    type = LineValueSampler
    start_point = '-0.611854 -1.883092 ${surface_profile_z}'
    end_point = '0.611854 1.883092 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
  [surface_line_05]
    type = LineValueSampler
    start_point = '0.0 -1.98 ${surface_profile_z}'
    end_point = '0.0 1.98 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
  [surface_line_06]
    type = LineValueSampler
    start_point = '0.611854 -1.883092 ${surface_profile_z}'
    end_point = '-0.611854 1.883092 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
  [surface_line_07]
    type = LineValueSampler
    start_point = '1.163815 -1.601854 ${surface_profile_z}'
    end_point = '-1.163815 1.601854 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
  [surface_line_08]
    type = LineValueSampler
    start_point = '1.601854 -1.163815 ${surface_profile_z}'
    end_point = '-1.601854 1.163815 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
  [surface_line_09]
    type = LineValueSampler
    start_point = '1.883092 -0.611854 ${surface_profile_z}'
    end_point = '-1.883092 0.611854 ${surface_profile_z}'
    num_points = ${surface_profile_num_points}
    sort_by = id
    variable = 'ux uy uz phi_cell_aux ke_aux gate_total_aux press_aux'
    outputs = 'surface_profile_csv'
  []
[]

[Postprocessors]
  [avg_n_analysis]
    type = ElementAverageValue
    variable = n
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_phi_cell_analysis]
    type = ElementAverageValue
    variable = phi_cell_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_phi_cell_nutr_analysis]
    type = ElementAverageValue
    variable = phi_cell_nutr_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_ke_analysis]
    type = ElementAverageValue
    variable = ke_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_kh_analysis]
    type = ElementAverageValue
    variable = kh_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_fa_analysis]
    type = ElementAverageValue
    variable = fa_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_gp_analysis]
    type = ElementAverageValue
    variable = gp_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_press_analysis]
    type = ElementAverageValue
    variable = press_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_J_analysis]
    type = ElementAverageValue
    variable = J_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_D_phys_analysis]
    type = ElementAverageValue
    variable = D_phys_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [avg_n_sink_analysis]
    type = ElementAverageValue
    variable = n_sink_aux
    execute_on = 'initial timestep_end'
    outputs = 'analysis_csv'
  []
  [V_over_V0]
    type = ParsedPostprocessor
    expression = 'avg_J_analysis'
    pp_names = 'avg_J_analysis'
    outputs = 'analysis_csv'
  []
  [dV_over_V0]
    type = ParsedPostprocessor
    expression = 'avg_J_analysis - 1'
    pp_names = 'avg_J_analysis'
    outputs = 'analysis_csv'
  []
  [dV_percent]
    type = ParsedPostprocessor
    expression = '100 * (avg_J_analysis - 1)'
    pp_names = 'avg_J_analysis'
    outputs = 'analysis_csv'
  []
[]

[Outputs]
  [analysis_csv]
    type = CSV
    file_base = outputs/3d/puck_analysis
    execute_on = 'initial timestep_end final'
    show = 'avg_n_analysis avg_phi_cell_analysis avg_phi_cell_nutr_analysis avg_ke_analysis avg_kh_analysis avg_fa_analysis avg_gp_analysis avg_press_analysis avg_J_analysis avg_D_phys_analysis avg_n_sink_analysis V_over_V0 dV_over_V0 dV_percent'
  []
  [surface_profile_csv]
    type = CSV
    file_base = outputs/3d/puck_surface_profile
    execute_on = none
    execute_vector_postprocessors_on = 'initial timestep_end final'
    min_simulation_time_interval = ${surface_profile_interval}
    show = 'surface_line_00 surface_line_01 surface_line_02 surface_line_03 surface_line_04 surface_line_05 surface_line_06 surface_line_07 surface_line_08 surface_line_09'
    time_data = true
  []
[]

`puck.i` can be augmented with `ant_3d_profiles_analysis.i` to export MATLAB-friendly
surface profiles and analysis metrics.

- Profile strategy: 10 diameters on the top surface using `LineValueSampler`
- Coverage: 20 radial directions via 10 center-crossing diameters
- Timing: rate-limited by simulation time, default `surface_profile_interval = 2.0`
- Sampled line fields:
  - `ux uy uz`
  - `phi_cell_aux`
  - `ke_aux`
  - `gate_total_aux`
  - `press_aux`

Recommended run-folder layout mirrors the successful 2D case:

- main outputs:
  - `.../puck.e`
  - `.../puck.csv`
  - `.../puck_solver_watch.csv`
  - `.../puck_analysis.csv`
- profile outputs:
  - `.../puck_surface_profile_surface_line_00_0000.csv`
  - `.../puck_surface_profile_surface_line_01_0000.csv`
  - ...
  - `.../puck_surface_profile_surface_line_09_0000.csv`
  - plus one time index file per line:
    - `.../puck_surface_profile_surface_line_00_time.csv`
    - ...

This follows the same pattern as the 2D successful run:

- `rz_surface_profile_surface_top_0000.csv`
- `rz_surface_profile_surface_top_time.csv`

For MATLAB/FFT:

- line CSVs provide reference coordinates `x,y,z`, distance `id`, and sampled values
- deformed line coordinates are reconstructed as:
  - `X = x + ux`
  - `Y = y + uy`
  - `Z = z + uz`
- the height profile for a line is therefore available directly from `Z`

Suggested future launch pattern:

```bash
./ant-opt -i inputs/3d/puck.i \
  inputs/solver_overrides/ant_prod_fsp_3d.i \
  inputs/solver_overrides/ant_3d_checkpoint.i \
  inputs/solver_overrides/ant_3d_caseA_match2d.i \
  inputs/solver_overrides/ant_3d_profiles_analysis.i \
  mesh_file=/abs/path/to/puck.msh \
  phi_file=/abs/path/to/phi_match2d_caseA_aphi002.e \
  Outputs/exodus/file_base=/abs/run_dir/puck \
  Outputs/csv/file_base=/abs/run_dir/puck \
  Outputs/solver_watch/file_base=/abs/run_dir/puck_solver_watch \
  Outputs/analysis_csv/file_base=/abs/run_dir/puck_analysis \
  Outputs/surface_profile_csv/file_base=/abs/run_dir/puck_surface_profile \
  Outputs/chk/file_base=/abs/run_dir/puck_chk
```

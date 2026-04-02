`rz_static_run.i` now exports the deformed top free surface for FFT.

- Boundary sampled: `top`
- VectorPostprocessor: `surface_top` (`SideValueSampler`)
- Coordinates are deformed coordinates because `use_displaced_mesh = true`
- Raw files:
  - `${surface_profile_file_base}_surface_top_0000.csv`, `${surface_profile_file_base}_surface_top_0001.csv`, ...
  - `${surface_profile_file_base}_surface_top_time.csv`
- Timing:
  - the profile export runs at `timestep_end`
  - output is rate-limited by `surface_profile_interval` in simulation time
  - default `surface_profile_interval = 1.0`, so files are written about once per simulation second without forcing the timestepper
  - exact physical output times are recorded in `${surface_profile_file_base}_surface_top_time.csv`

Run the cleanup step with:

```bash
python scripts/export_surface_profile_xy.py \
  "outputs/.../rz_surface_profile_surface_top_*.csv"
```

This writes one sorted `x,y` CSV per timestep in a sibling `surface_xy_profiles/` directory.

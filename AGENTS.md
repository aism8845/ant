# AGENTS.md — `ant` working handoff and implementation guide

This file is the root handoff for `/home/amcgs/projects/ant`.

It has two purposes:
- give future agents a precise working model of the app
- record the design and solver lessons transferred from `collie`, `CellGel`, `buckling`, and `biofilm`

This document is intentionally technical. Treat it as the authoritative local guide for this repo.

---

## 1. Scope and working rules

### Repo scope
- Work in this repo only: `/home/amcgs/projects/ant`
- Do not edit `collie`, `CellGel`, `buckling`, or `biofilm` unless the user explicitly asks for cross-repo edits.
- Those repos are donors and references, not write targets for ordinary `ant` work.

### Non-negotiables
- Use `SolidMechanics` new system only.
- Keep mechanics on the non-AD Total Lagrangian stress path unless the user explicitly requests a full AD mechanics rewrite.
- Keep nutrient transport AD.
- Do not mix AD and non-AD properties with the same semantic role/name.
- Do not add Riks / arc-length / custom continuation machinery by default.
- Do not “fix” convergence by changing constitutive equations unless the user explicitly asks for physics changes.

### Working style
- Make minimal diffs.
- Prefer donor reuse over fresh invention.
- Preserve sign conventions and property names once established.
- Validate with build + smoke before declaring success.

### Minimum validation before handoff
- `cd /home/amcgs/projects/ant && make -j8`
- `cd /home/amcgs/projects/ant && BUILD=0 ./scripts/smoke_ant.sh`
- If new inputs are added:
  - run `--check-input`
  - run at least one short real solve

---

## 2. What `ant` is

`ant` is a minimal MOOSE app for a fully coupled cell/gel mixture with nutrient transport.

The current architecture is a hybrid one:
- **mechanics:** non-AD, Total Lagrangian, `ComputeLagrangianStressPK2`
- **nutrient transport:** AD
- **mechanics ← nutrient off-diagonal:** explicit via `d(pk1_stress)/dn`

This is deliberate. It follows the path that was actually workable across the donor apps.

The app is not meant to be a generic continuation framework. It is a coupled constitutive and PDE sandbox for:
- CellGel-style mixture mechanics
- nutrient-driven growth coupling
- Jacobian-consistent block coupling
- 2D axisymmetric and 3D puck-style problems

---

## 3. Donor map and what came from each repo

### `CellGel` — primary constitutive base
Use `CellGel` as the main constitutive source of truth.

Main donors:
- `CellNeoHook`
- `PolyGentYield`
- `CellGelMixtureNut2`
- recent production solver stacks

What `CellGel` contributes:
- midpoint objective update style
- stateful internal metrics
- finite-difference algorithmic tangent in `computeQpPK2Stress()`
- current “production” solver defaults for RZ and 3D RVE runs

### `buckling` — donor only for coupling pattern ideas
Use `buckling` only as a donor of:
- nutrient gate shape `fa(n)`
- pressure gate shape `gp(p_cell)`
- robust centered finite-difference pattern for `dpk1_stress_dn`
- solver preset summaries

Do **not** import:
- Riks
- snap-through / path-following infrastructure
- solver-specific special machinery

### `biofilm` — donor for AD transport and coupled Jacobian discipline
Use `biofilm` for:
- `ADNutrientTLTransport`
- `ADJnTimeDerivative`
- `ADMatTensorDiffusion`
- `ADMatReactionSigned`
- off-diagonal coupling patterns
- Jacobian validation discipline
- bath-calibration solver lessons

Most important `biofilm` lessons:
- keep transport fully AD
- keep mechanics TL/new-system
- inject mechanics ← field off-diagonal explicitly
- validate Jacobian quality routinely

### `collie` — donor for RZ implementation details and phi-ref usage
Use `collie` for:
- axisymmetric RZ geometry-specific transport path
- `TLStressDivergenceNutrientOffDiag` on `TotalLagrangianStressDivergenceAxisymmetricCylindrical`
- `phi_ref_ic` / `phi_cell_ref` handling patterns
- puck / phi-ref filter input structure

Most important `collie` lesson:
- for RZ, use geometry-aware transport and mechanics kernels; Cartesian 2D logic is not theoretically correct in axisymmetry

---

## 4. Current `ant` architecture

### Mechanics side

Main files:
- `include/base/materials/CellNeoHook.h`
- `src/materials/CellNeoHook.C`
- `include/base/materials/PolyGentYield.h`
- `src/materials/PolyGentYield.C`
- `include/base/materials/CellGelMixtureNut2.h`
- `src/materials/CellGelMixtureNut2.C`

The main mixture material is:
- `CellGelMixtureNut2`

This is the primary mechanics material for coupled mixture work in `ant`.

### Nutrient side

Cartesian transport:
- `include/base/materials/ADNutrientTLTransport.h`
- `src/materials/ADNutrientTLTransport.C`

Axisymmetric transport:
- `include/base/materials/ADNutrientTLTransportRZ.h`
- `src/materials/ADNutrientTLTransportRZ.C`

AD transport kernels:
- `include/kernels/ADJnTimeDerivative.h`
- `src/kernels/ADJnTimeDerivative.C`
- `include/kernels/ADMatTensorDiffusion.h`
- `src/kernels/ADMatTensorDiffusion.C`
- `include/kernels/ADMatReactionSigned.h`
- `src/kernels/ADMatReactionSigned.C`

### Mechanics ← nutrient off-diagonal

Cartesian:
- `include/kernels/TLStressDivergenceNutrientOffDiag.h`
- `src/kernels/TLStressDivergenceNutrientOffDiag.C`

Axisymmetric:
- `include/kernels/TLStressDivergenceNutrientOffDiagRZ.h`
- `src/kernels/TLStressDivergenceNutrientOffDiagRZ.C`

These consume:
- `getMaterialPropertyDerivative<RankTwoTensor>(_base_name + "pk1_stress", coupledName("n"))`

They are intentionally explicit and non-AD on the mechanics side.

---

## 5. Physics model in `ant`

### 5.1 Cell constitutive law

The cell phase is based on compressible neo-Hookean mechanics:
- shear modulus `G_cell`
- bulk modulus `K_cell`

Growth is gated by:
- nutrient gate `fa(n)`
- pressure gate `gp(p_cell)`

The current cell growth rate is:
- `ke = ke_drive * fa * gp`
  with gates toggled by:
  - `gate_fa_on_ke`
  - `gate_gp_on_ke`

### 5.2 Gel constitutive law

The gel phase is Gent-like:
- shear modulus `G_gel`
- limiting invariant `Im`

Relaxation is Ellis/yield style:
- low-stress rate `kD0`
- high-stress rate `kDinf`
- yield scale `sig_yield`
- exponent `n_kD`

### 5.3 Mixture stress

The Cauchy mixture stress is phase-averaged:
- `sigma_mix = phi_cell * sigma_cell + (1 - phi_cell) * sigma_gel`

The PK1/PK2 tangent is built by finite difference inside:
- `CellGelMixtureNut2::computeQpPK2Stress()`

This is not exact AD mechanics. It is a hybrid consistent tangent.

### 5.4 Nutrient transport

Transport is total-Lagrangian:
- time coefficient `J_nutr`
- time-rate coefficient `Jdot_nutr`
- referential diffusion tensor `D_eff_nutr = J D F^{-1} F^{-T}`

Crowding is handled through current cell fraction `phi`.

### 5.5 Nutrient consumption sign convention

This is now fixed and must stay fixed.

Rule:
- `ADMatReactionSigned` contributes residual `-reaction_rate * u`

Therefore:
- `n_source_ref_nutr` must be a **positive sink coefficient**
- not a signed source term

Current implementation:
- `ADNutrientTLTransport.C` sets `n_source_ref_nutr = J * gamma_local`
- `ADNutrientTLTransportRZ.C` sets `n_source_ref_nutr = J_eff * gamma_local`

Do not revert that sign unless you also change the reaction kernel convention.

---

## 6. The critical constitutive consistency fix

This was the biggest defect discovered in the first `ant` round.

### Old incorrect state
The model used two different closures for cell fraction:
- mechanics evolved current `phi_cell` directly via an ODE
- transport reconstructed current `phi` from `phi_ref` and `J`

That is inconsistent.

### Current correct rule in `ant`
There is now one consistent closure:

- the **kinetic state** is `phi_cell_ref`
- the **current fraction** is reconstructed from `phi_ref` and `J`

Current formula:

- `phi = J * phi_ref / ( (J - 1) * phi_ref + 1 )`

Implementation:
- `CellGelMixtureNut2.C` evolves `phi_cell_ref`
- `CellGelMixtureNut2.C` reconstructs `phi_cell` using `phiFromRefAndJ(...)`
- `ADNutrientTLTransport*.C` reconstruct current `phi` from the same `phi_ref` and `J`

This is the most important constitutive rule to preserve.

### Practical consequence
If you change `phi` evolution in mechanics, you must update transport consistently.
If you change `phi_ref` handling in transport, you must update mechanics consistently.

Do not let these drift apart again.

---

## 7. Geometry-specific support

### 3D Cartesian
Use:
- `ADNutrientTLTransport`
- `TLStressDivergenceNutrientOffDiag`

This is correct for 3D Cartesian puck/cube-style problems.

### 2D axisymmetric RZ
Use:
- `ADNutrientTLTransportRZ`
- `TLStressDivergenceNutrientOffDiagRZ`

Why:
- RZ needs hoop stretch `F_theta_theta = 1 + u_r / r`
- Cartesian 2D transport is not theoretically correct in axisymmetric geometry

Do not use the Cartesian transport/offdiag pair in an RZ input.

---

## 8. Current validated input structure

### 2D folder
Path:
- `inputs/2d/`

Files:
- `inputs/2d/rz.geo`
  - gmsh mesh for axisymmetric rectangle/puck-style domain
  - refined near:
    - outer radial shell
    - top surface
    - top-right buckling corner
- `inputs/2d/phi.i`
  - filtered random `phi_ref_ic` speckle on the same mesh
- `inputs/2d/rz.i`
  - main coupled axisymmetric mechanics + nutrient input

### 3D folder
Path:
- `inputs/3d/`

Files:
- `inputs/3d/puck.geo`
  - gmsh 3D puck mesh
  - refined near the full boundary shell
- `inputs/3d/phi.i`
  - filtered random `phi_ref_ic` field on the puck mesh
- `inputs/3d/puck.i`
  - main coupled 3D puck input

### Speckle workflow
Both 2D and 3D use the same workflow:
1. generate mesh with `gmsh`
2. run `phi.i` to create `phi_ref_ic` Exodus
3. run main input loading `phi_ref_ic` through:
   - `SolutionUserObject`
   - `SolutionIC`

This is the correct path for reproducible speckled reference fraction fields.

---

## 9. Mesh generation commands

From repo root:

### 2D axisymmetric mesh
```bash
cd /home/amcgs/projects/ant
gmsh inputs/2d/rz.geo -2 -format msh2 -o inputs/2d/rz.msh
```

### 3D puck mesh
```bash
cd /home/amcgs/projects/ant
gmsh inputs/3d/puck.geo -3 -format msh2 -o inputs/3d/puck.msh
```

Notes:
- `rz.geo` uses local refinement near the expected buckling/stress-concentration region.
- `puck.geo` uses boundary-shell refinement, which is the simplest efficient 3D analogue for current use.
- `puck.geo` may print a gmsh warning about an unknown surface orientation during generation; in the current validated run this still produced a valid mesh and `0` gmsh errors.

---

## 10. Solver guidance

### Current default stack in `ant`
Use this unless there is a clear reason not to:
- `Transient`
- `scheme = bdf2`
- `solve_type = NEWTON`
- `line_search = bt`
- `automatic_scaling = true`
- PETSc:
  - `newtonls`
  - `preonly`
  - `lu`
  - `mumps`
- adaptive stepping:
  - `dt = 0.02`
  - `dtmin = 1e-3`
  - `growth_factor = 1.05`
  - `cutback_factor = 0.5`
  - `optimal_iterations = 6`

This is the current “balanced production” stack transferred from the recent CellGel RVE sweeps.

### Why this stack
Lessons from `CellGel`:
- large-deformation soft corners are rarely limited by the linear solve
- the main difficulty is nonlinear continuation and aggressive timestep growth
- lowering `growth_factor` and `optimal_iterations` helps more than adding solver complexity

### Speed / accuracy presets inherited from donor repos

#### Max speed
From `buckling`:
- SMP + LU/MUMPS
- `automatic_scaling = true`
- speed-oriented overrides can be useful for moderate problems

#### Balanced default
From `buckling` + `CellGel`:
- lockstep-ish NewtonLS/BT with modest adaptive growth
- good default for production trajectories

#### Max accuracy / reference
From `buckling`:
- tighter lockstep/reference stacks are slower but useful for parity studies

### Negative guidance
Do not do these unless explicitly instructed:
- PETSc arc-length by input flags alone
- custom continuation hosts
- Riks / snap-through machinery

The donor history is clear:
- those tools were often not actually wired in the local MOOSE path
- they added complexity without solving the dominant issues in routine runs

---

## 11. Jacobian and coupling expectations

### What “fully coupled” means here
This app is fully coupled in **block structure**, not fully exact AD everywhere.

Current state:
- nutrient block: exact AD
- mechanics block: non-AD with finite-difference consistent tangent
- mechanics ← nutrient off-diagonal: finite-difference `dPK1/dn`

This is acceptable and intentional for current `ant`.

### Jacobian test target
- target agreement: about `1e-7`
- acceptable smoke threshold: `<= 1e-6`

Current validated smoke:
- Jacobian ratio `~1.3e-7`

### Required check when touching mechanics/nutrient coupling
Run:
```bash
cd /home/amcgs/projects/ant
BUILD=0 ./scripts/smoke_ant.sh
```

If the Jacobian ratio regresses materially, stop and investigate.

---

## 12. Current validated status (as of 2026-03-25)

### Build
- `make -j8` passed

### Baseline smoke
- `BUILD=0 ./scripts/smoke_ant.sh` passed

### Current validated runs

#### 2D filter
- `inputs/2d/phi.i` runs successfully

#### 2D axisymmetric main
- `inputs/2d/rz.i`:
  - input-check passes
  - short real run passes
  - solver converges cleanly with:
    - mostly `1–2` nonlinear iterations
    - `1` linear iteration per Newton step

#### 3D filter
- `inputs/3d/phi.i` runs successfully

#### 3D puck main
- `inputs/3d/puck.i`:
  - input-check passes
  - short real run passes

### Important note on a confusing startup message
MOOSE may print:
- `SolidMechanics Action: selecting 'incremental finite strain' formulation.`

This message is stale/misleading in this path.
The inputs are still using:
- `new_system = true`
- `strain = FINITE`
- `formulation = TOTAL`

Treat the input settings as authoritative, not that action message.

---

## 13. Practical pitfalls already hit

### Mesh path resolution
MOOSE resolves file paths relative to the input file directory.

That means:
- in `inputs/2d/rz.i`, `mesh_file = inputs/2d/rz.msh` is wrong
- it resolves to `inputs/2d/inputs/2d/rz.msh`

Current safe rule:
- use local filenames like `rz.msh`, `puck.msh`
- or use absolute paths

### Speckle file loading
Same issue applies to:
- `SolutionUserObject mesh = ...`

Use paths that are correct relative to the input file location.

### Do not mix Cartesian and RZ transport
If you add a new RZ input, use:
- `ADNutrientTLTransportRZ`
- `TLStressDivergenceNutrientOffDiagRZ`

### Do not reintroduce signed nutrient source confusion
Remember:
- `ADMatReactionSigned` multiplies by `-u`
- `n_source_ref_nutr` must therefore be a positive sink coefficient

---

## 14. Recommended next-step workflow for future agents

When adding or modifying a case:

1. Decide geometry class first
   - 3D Cartesian or 2D RZ
2. Choose the correct transport/offdiag pair
3. Preserve `phi_ref` + `J` closure consistency
4. Keep the solver stack simple and current
5. Build the mesh
6. Run the `phi.i` speckle file
7. Run `--check-input`
8. Run a short real solve
9. Run `smoke_ant.sh` if constitutive or coupling code changed

---

## 15. File map

### Core materials
- `include/base/materials/CellNeoHook.h`
- `src/materials/CellNeoHook.C`
- `include/base/materials/PolyGentYield.h`
- `src/materials/PolyGentYield.C`
- `include/base/materials/CellGelMixtureNut2.h`
- `src/materials/CellGelMixtureNut2.C`

### Nutrient transport
- `include/base/materials/ADNutrientTLTransport.h`
- `src/materials/ADNutrientTLTransport.C`
- `include/base/materials/ADNutrientTLTransportRZ.h`
- `src/materials/ADNutrientTLTransportRZ.C`

### Kernels
- `include/kernels/ADJnTimeDerivative.h`
- `src/kernels/ADJnTimeDerivative.C`
- `include/kernels/ADMatTensorDiffusion.h`
- `src/kernels/ADMatTensorDiffusion.C`
- `include/kernels/ADMatReactionSigned.h`
- `src/kernels/ADMatReactionSigned.C`
- `include/kernels/TLStressDivergenceNutrientOffDiag.h`
- `src/kernels/TLStressDivergenceNutrientOffDiag.C`
- `include/kernels/TLStressDivergenceNutrientOffDiagRZ.h`
- `src/kernels/TLStressDivergenceNutrientOffDiagRZ.C`

### Inputs
- `inputs/current/smoke.i`
- `inputs/2d/phi.i`
- `inputs/2d/rz.i`
- `inputs/2d/rz.geo`
- `inputs/3d/phi.i`
- `inputs/3d/puck.i`
- `inputs/3d/puck.geo`

### Docs
- `README.md`
- `notes/SOLVERS.md`
- `AGENTS.md`

---

## 16. Greps for quick verification

### Constitutive consistency
```bash
rg -n "phiFromRefAndJ|phi_cell_ref|phi_cell_0" /home/amcgs/projects/ant/src/materials/CellGelMixtureNut2.C
```

### Reaction sign
```bash
rg -n "n_source_ref_nutr|precomputeQpResidual" /home/amcgs/projects/ant/src/materials /home/amcgs/projects/ant/src/kernels/ADMatReactionSigned.C
```

### RZ support
```bash
rg -n "ADNutrientTLTransportRZ|TLStressDivergenceNutrientOffDiagRZ" /home/amcgs/projects/ant
```

### 2D inputs
```bash
rg -n "SolutionUserObject|phi_ref_from_file|IterationAdaptiveDT|growth_factor|optimal_iterations" /home/amcgs/projects/ant/inputs/2d
```

### 3D inputs
```bash
rg -n "SolutionUserObject|IterationAdaptiveDT|growth_factor|optimal_iterations" /home/amcgs/projects/ant/inputs/3d
```

---

## 17. Commit discipline

Before a large constitutive or coupling change:
- make a git commit

After the change is validated:
- make another git commit

Do not let large mechanics, transport, and input-structure edits accumulate without a checkpoint commit.


Ant
===

`ant` is a minimal MOOSE app for a fully coupled nutrient-mechanics CellGel-style mixture model.

Scope
-----
- Cell constitutive base: `CellNeoHook`
- Gel constitutive base: `PolyGentYield`
- Mixture scaffold: repaired `CellGelMixtureNut2`
- Nutrient transport: `ADNutrientTLTransport`
- Mechanics → nutrient: AD transport properties through `J_nutr`, `Jdot_nutr`, `D_eff_nutr`, `n_source_ref_nutr`
- Nutrient → mechanics: explicit `d(pk1_stress)/dn` through `TLStressDivergenceNutrientOffDiag`

Donor map
---------
- `CellGel`: main constitutive structure, midpoint update, stateful variables, PK2 tangent construction
- `buckling`: donor only for `fa(n)`, `gp(p_cell)`, and robust centered finite-difference `dpk1_stress_dn`
- `biofilm`: donor for the AD nutrient transport and kernel stack, plus coupled Jacobian validation pattern

Core files
----------
- `include/base/materials/CellGelMixtureNut2.h`
- `src/materials/CellGelMixtureNut2.C`
- `src/materials/ADNutrientTLTransport.C`
- `src/kernels/TLStressDivergenceNutrientOffDiag.C`
- `inputs/current/smoke.i`
- `scripts/smoke_ant.sh`
- `notes/SOLVERS.md`

What is intentionally not included
----------------------------------
- Riks / arc-length / continuation machinery
- custom nonlinear executioners
- broad sweep infrastructure
- unrelated CellGel phase-map and homogenization inputs

Build
-----
```bash
cd ant
make -j8
```

Smoke / Jacobian check
----------------------
```bash
cd ant
./scripts/smoke_ant.sh
```

The smoke script:
- builds the app
- runs a short coupled transient
- runs a PETSc Jacobian spot-check with `-snes_test_jacobian`
- fails if the Jacobian ratio is missing, diverges, or exceeds the configured tolerance

Solver guidance
---------------
See `notes/SOLVERS.md`.

Commit reminder
---------------
- Make a git commit before large constitutive changes.
- Make another git commit after validating the smoke/Jacobian path.

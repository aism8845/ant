SetFactory("OpenCASCADE");

// Symmetric all-surface refinement for 3D puck runs.
// Recommendation:
//   - keep the surface floor at 25 um to preserve ~10x surface sampling density
//     relative to the old 80 um puck mesh
//   - cut runtime by thinning the refined boundary band and coarsening the core
//
// Defaults here keep the 25 um surface floor but make the boundary layer much
// thinner so the whole-domain tet count stays practical.
//   lc_shell   = 0.025 mm
//   lc_bulk    = 1.00  mm
//   shell_band = 0.05  mm
//   trans_band = 0.10  mm

If (!Exists(radius))
  radius = 2.0;
EndIf
If (!Exists(height))
  height = 1.0;
EndIf
If (!Exists(lc_shell))
  lc_shell = 0.025;
EndIf
If (!Exists(lc_bulk))
  lc_bulk = 1.00;
EndIf
If (!Exists(shell_band))
  shell_band = 0.05;
EndIf
If (!Exists(trans_band))
  trans_band = 0.10;
EndIf

Cylinder(1) = {0.0, 0.0, 0.0, 0.0, 0.0, height, radius};
bnd[] = Boundary{ Volume{1}; };

top[] = Surface In BoundingBox{
  -radius - 0.1, -radius - 0.1, height - 0.01,
   radius + 0.1,  radius + 0.1, height + 0.01
};

bot[] = Surface In BoundingBox{
  -radius - 0.1, -radius - 0.1, -0.01,
   radius + 0.1,  radius + 0.1, 0.01
};

side[] = {};
For i In {0:#bnd[]-1}
  is_top = 0;
  is_bot = 0;
  For j In {0:#top[]-1}
    If (bnd[i] == top[j])
      is_top = 1;
    EndIf
  EndFor
  For j In {0:#bot[]-1}
    If (bnd[i] == bot[j])
      is_bot = 1;
    EndIf
  EndFor
  If (!is_top && !is_bot)
    side[] += {bnd[i]};
  EndIf
EndFor

Physical Volume("puck", 1) = {1};
Physical Surface("boundary", 11) = {bnd[]};
Physical Surface("top", 12) = {top[]};
Physical Surface("bottom", 13) = {bot[]};
Physical Surface("side", 14) = {side[]};

Field[1] = Distance;
Field[1].FacesList = {bnd[]};
Field[1].Sampling = 100;

Field[2] = Threshold;
Field[2].IField = 1;
Field[2].LcMin = lc_shell;
Field[2].LcMax = lc_bulk;
Field[2].DistMin = shell_band;
Field[2].DistMax = shell_band + trans_band;
Field[2].StopAtDistMax = 1;

Background Field = 2;

Mesh.MeshSizeExtendFromBoundary = 0;
Mesh.MeshSizeFromCurvature = 0;
Mesh.MeshSizeFromPoints = 0;
Mesh.MeshSizeMin = lc_shell;
Mesh.MeshSizeMax = lc_bulk;
Mesh.Optimize = 0;
Mesh.Algorithm3D = 10;
Mesh.OptimizeNetgen = 0;
Mesh.Smoothing = 1;
Mesh.CharacteristicLengthFactor = 1.0;

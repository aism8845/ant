SetFactory("OpenCASCADE");

// Refined 3D puck mesh:
// - strong refinement on the top surface for wrinkle resolution
// - moderate refinement on side/bottom
// - aggressive bulk coarsening

If (!Exists(radius))
  radius = 2.0;
EndIf
If (!Exists(height))
  height = 1.0;
EndIf

// Top surface resolution
If (!Exists(lc_top))
  lc_top = 0.025;
EndIf

// Side/bottom resolution
If (!Exists(lc_side))
  lc_side = 0.10;
EndIf

// Deep interior resolution
If (!Exists(lc_bulk))
  lc_bulk = 0.50;
EndIf

// Top refinement depth/transition
If (!Exists(shell_top))
  shell_top = 0.15;
EndIf
If (!Exists(trans_top))
  trans_top = 0.30;
EndIf

// Side/bottom refinement depth/transition
If (!Exists(shell_side))
  shell_side = 0.10;
EndIf
If (!Exists(trans_side))
  trans_side = 0.20;
EndIf

Cylinder(1) = {0.0, 0.0, 0.0, 0.0, 0.0, height, radius};
bnd[] = Boundary{ Volume{1}; };

// Top surface (z = height)
top[] = Surface In BoundingBox{
  -radius - 0.1, -radius - 0.1, height - 0.01,
   radius + 0.1,  radius + 0.1, height + 0.01
};

// Bottom surface (z = 0)
bot[] = Surface In BoundingBox{
  -radius - 0.1, -radius - 0.1, -0.01,
   radius + 0.1,  radius + 0.1,  0.01
};

// Side surface = all boundary surfaces minus top and bottom
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

// Distance from top surface
Field[1] = Distance;
Field[1].FacesList = {top[]};
Field[1].Sampling = 100;

// Fine on top shell, transition to bulk
Field[2] = Threshold;
Field[2].IField = 1;
Field[2].LcMin = lc_top;
Field[2].LcMax = lc_bulk;
Field[2].DistMin = shell_top;
Field[2].DistMax = shell_top + trans_top;
Field[2].StopAtDistMax = 1;

// Distance from side + bottom
Field[3] = Distance;
Field[3].FacesList = {side[], bot[]};
Field[3].Sampling = 100;

// Moderate on side/bottom shell, transition to bulk
Field[4] = Threshold;
Field[4].IField = 3;
Field[4].LcMin = lc_side;
Field[4].LcMax = lc_bulk;
Field[4].DistMin = shell_side;
Field[4].DistMax = shell_side + trans_side;
Field[4].StopAtDistMax = 1;

// Combine top and side/bottom constraints
Field[5] = Min;
Field[5].FieldsList = {2, 4};

Background Field = 5;

Mesh.MeshSizeExtendFromBoundary = 0;
Mesh.MeshSizeFromCurvature = 0;
Mesh.MeshSizeFromPoints = 0;
Mesh.MeshSizeMin = lc_top;
Mesh.MeshSizeMax = lc_bulk;
Mesh.Algorithm3D = 10;
Mesh.OptimizeNetgen = 1;
Mesh.Smoothing = 3;
Mesh.CharacteristicLengthFactor = 1.0;

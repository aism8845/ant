// square_prism_40k.geo
// Nominal-length comparison geometry:
//   square prism, side length 4, thickness 1
//
// Physical group names intentionally match inputs/3d/puck.i:
//   Volume("puck"), Surface("boundary"), Surface("top"),
//   Surface("bottom"), Surface("side")

SetFactory("OpenCASCADE");

side_length = 4.0;
height = 1.0;
half = side_length / 2;

Box(1) = {-half, -half, 0, side_length, side_length, height};

bnd_surfs[] = Boundary{ Volume{1}; };

top[] = Surface In BoundingBox{
    -half - 0.1, -half - 0.1, height - 0.01,
     half + 0.1,  half + 0.1, height + 0.01
};

bot[] = Surface In BoundingBox{
    -half - 0.1, -half - 0.1, -0.01,
     half + 0.1,  half + 0.1,  0.01
};

side[] = {};
For i In {0:#bnd_surfs[]-1}
  is_top = 0;
  is_bot = 0;
  For j In {0:#top[]-1}
    If (bnd_surfs[i] == top[j])
      is_top = 1;
    EndIf
  EndFor
  For j In {0:#bot[]-1}
    If (bnd_surfs[i] == bot[j])
      is_bot = 1;
    EndIf
  EndFor
  If (!is_top && !is_bot)
    side[] += {bnd_surfs[i]};
  EndIf
EndFor

Physical Volume("puck") = {1};
Physical Surface("boundary") = {bnd_surfs[]};
Physical Surface("top") = {top[]};
Physical Surface("bottom") = {bot[]};
Physical Surface("side") = {side[]};

// Slightly coarser than the puck to offset the larger 4x4 footprint.
lc_shell = 0.055;
lc_bulk = 0.132;
shell_band = 0.08;
trans_band = 0.42;

Field[1] = MathEval;
Field[1].F = Sprintf("Min(Min(z, %g - z), Min(%g - Abs(x), %g - Abs(y)))",
                     height, half, half);

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

Mesh.Algorithm3D = 10;
Mesh.OptimizeNetgen = 1;
Mesh.Smoothing = 3;

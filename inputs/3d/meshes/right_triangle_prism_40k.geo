// right_triangle_prism_40k.geo
// Nominal-length comparison geometry:
//   right-isosceles triangle prism, legs 4, 4, thickness 1
//
// The triangle is translated so its centroid is at the origin. This keeps the
// existing puck.i pin points (0,0,0.5), (1,0,0.5), and (0,1,0.5) inside the mesh.
//
// Physical group names intentionally match inputs/3d/puck.i.

SetFactory("OpenCASCADE");

leg = 4.0;
height = 1.0;
shift = leg / 3;

Point(1) = {-shift, -shift, 0};
Point(2) = {leg - shift, -shift, 0};
Point(3) = {-shift, leg - shift, 0};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 1};
Curve Loop(1) = {1, 2, 3};
Plane Surface(1) = {1};

extruded[] = Extrude {0, 0, height} { Surface{1}; };

vols[] = Volume In BoundingBox{
  -shift - 0.1, -shift - 0.1, -0.01,
   leg - shift + 0.1, leg - shift + 0.1, height + 0.01
};

bnd_surfs[] = Boundary{ Volume{vols[]}; };

top[] = Surface In BoundingBox{
  -shift - 0.1, -shift - 0.1, height - 0.01,
   leg - shift + 0.1, leg - shift + 0.1, height + 0.01
};

bot[] = Surface In BoundingBox{
  -shift - 0.1, -shift - 0.1, -0.01,
   leg - shift + 0.1, leg - shift + 0.1, 0.01
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

Physical Volume("puck") = {vols[]};
Physical Surface("boundary") = {bnd_surfs[]};
Physical Surface("top") = {top[]};
Physical Surface("bottom") = {bot[]};
Physical Surface("side") = {side[]};

// Slightly finer than the puck to offset the smaller 4x4 right-triangle footprint.
lc_shell = 0.045;
lc_bulk = 0.109;
shell_band = 0.08;
trans_band = 0.42;

// Distances to the three in-plane edges for the centroid-centered right triangle:
//   x >= -leg/3
//   y >= -leg/3
//   x + y <= leg/3
Field[1] = MathEval;
Field[1].F = Sprintf("Min(Min(z, %g - z), Min(x + %g, Min(y + %g, (%g - x - y)/%g)))",
                     height, shift, shift, shift, Sqrt(2));

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

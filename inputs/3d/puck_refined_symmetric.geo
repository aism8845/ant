// puck_refined_symmetric.geo
// Symmetric surface refinement, aggressive interior coarsening.
// Tuned for ~20,000 nodes total.
//
// FIX: Uses MathEval field with analytical distance-to-boundary
// instead of Distance field, which only measured from edges/curves
// and left the disk centers coarse.
//
// For a cylinder (R=2, H=1, centered at origin, axis along Z):
//   dist_to_boundary = min(z, H-z, R - sqrt(x^2+y^2))
// This is exact and works during both 2D and 3D meshing.

SetFactory("OpenCASCADE");

// Geometry
If (!Exists(radius))
  radius = 2.0;
EndIf
If (!Exists(height))
  height = 1.0;
EndIf
Cylinder(1) = {0, 0, 0, 0, 0, height, radius};

// Identify boundary surfaces
bnd_surfs[] = Boundary{ Volume{1}; };

top[] = Surface In BoundingBox{
    -radius-0.1, -radius-0.1, height-0.01,
     radius+0.1,  radius+0.1, height+0.01};

bot[] = Surface In BoundingBox{
    -radius-0.1, -radius-0.1, -0.01,
     radius+0.1,  radius+0.1,  0.01};

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

// Physical groups
Physical Volume("puck") = {1};
Physical Surface("boundary") = {bnd_surfs[]};
Physical Surface("top") = {top[]};
Physical Surface("bottom") = {bot[]};
Physical Surface("side") = {side[]};

// Mesh size parameters retuned for the analytical boundary-distance field.
// The exact 0.04 shell size over-refines the full top/bottom disks in this
// scheme, so use a coarser surface floor to stay in the desired node range.
If (!Exists(lc_shell))
  lc_shell = 0.06;
EndIf
If (!Exists(lc_bulk))
  lc_bulk = 0.40;
EndIf
If (!Exists(shell_band))
  shell_band = 0.12;
EndIf
If (!Exists(trans_band))
  trans_band = 0.25;
EndIf

// Field 1: Analytical distance to nearest boundary
// For cylinder: min(z, H-z, R-sqrt(x^2+y^2))
Field[1] = MathEval;
Field[1].F = Sprintf("Min(Min(z, %g - z), %g - Sqrt(x^2 + y^2))", height, radius);

// Field 2: Map distance to mesh size
Field[2] = Threshold;
Field[2].IField = 1;
Field[2].LcMin = lc_shell;
Field[2].LcMax = lc_bulk;
Field[2].DistMin = shell_band;
Field[2].DistMax = shell_band + trans_band;
Field[2].StopAtDistMax = 1;

Background Field = 2;

// Mesh options
Mesh.MeshSizeExtendFromBoundary = 0;
Mesh.MeshSizeFromCurvature = 0;
Mesh.MeshSizeFromPoints = 0;
Mesh.MeshSizeMin = lc_shell;
Mesh.MeshSizeMax = lc_bulk;

Mesh.Algorithm3D = 10;
Mesh.OptimizeNetgen = 1;
Mesh.Smoothing = 3;

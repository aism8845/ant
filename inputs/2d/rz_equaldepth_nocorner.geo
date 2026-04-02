SetFactory("OpenCASCADE");

If (!Exists(radius))
  radius = 2.0;
EndIf
If (!Exists(height))
  height = 0.5;
EndIf
If (!Exists(lc_bulk))
  lc_bulk = 0.04;
EndIf
If (!Exists(lc_boundary))
  lc_boundary = 0.006;
EndIf
If (!Exists(band_depth))
  band_depth = 0.50;
EndIf

Point(1) = {0.0, 0.0, 0.0, lc_boundary};
Point(2) = {radius, 0.0, 0.0, lc_boundary};
Point(3) = {radius, height, 0.0, lc_boundary};
Point(4) = {0.0, height, 0.0, lc_boundary};

Line(1) = {1, 2};
Line(2) = {2, 3};
Line(3) = {3, 4};
Line(4) = {4, 1};

Curve Loop(10) = {1, 2, 3, 4};
Plane Surface(20) = {10};

Physical Surface("puck", 1) = {20};
Physical Curve("bottom", 11) = {1};
Physical Curve("right", 12) = {2};
Physical Curve("top", 13) = {3};
Physical Curve("left", 14) = {4};

Field[1] = Distance;
Field[1].CurvesList = {2};
Field[1].NumPointsPerCurve = 200;

Field[2] = Threshold;
Field[2].IField = 1;
Field[2].LcMin = lc_boundary;
Field[2].LcMax = lc_bulk;
Field[2].DistMin = 0.0;
Field[2].DistMax = band_depth;

Field[3] = Distance;
Field[3].CurvesList = {3};
Field[3].NumPointsPerCurve = 200;

Field[4] = Threshold;
Field[4].IField = 3;
Field[4].LcMin = lc_boundary;
Field[4].LcMax = lc_bulk;
Field[4].DistMin = 0.0;
Field[4].DistMax = band_depth;

Field[5] = Min;
Field[5].FieldsList = {2, 4};

Background Field = 5;

Mesh.MeshSizeExtendFromBoundary = 0;
Mesh.MeshSizeFromCurvature = 1;
Mesh.MeshSizeFromPoints = 1;
Mesh.Algorithm = 6;

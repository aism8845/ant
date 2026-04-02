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
If (!Exists(lc_shell))
  lc_shell = 0.012;
EndIf
If (!Exists(shell_band))
  shell_band = 0.10;
EndIf
If (!Exists(top_band))
  top_band = 0.08;
EndIf
If (!Exists(corner_band))
  corner_band = 0.18;
EndIf

Point(1) = {0.0, 0.0, 0.0, lc_shell};
Point(2) = {radius, 0.0, 0.0, lc_shell};
Point(3) = {radius, height, 0.0, lc_shell};
Point(4) = {0.0, height, 0.0, lc_shell};

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
Field[2].LcMin = lc_shell;
Field[2].LcMax = lc_bulk;
Field[2].DistMin = 0.0;
Field[2].DistMax = shell_band;

Field[3] = Distance;
Field[3].CurvesList = {3};
Field[3].NumPointsPerCurve = 200;

Field[4] = Threshold;
Field[4].IField = 3;
Field[4].LcMin = lc_shell;
Field[4].LcMax = lc_bulk;
Field[4].DistMin = 0.0;
Field[4].DistMax = top_band;

Field[5] = Box;
Field[5].VIn = lc_shell;
Field[5].VOut = lc_bulk;
Field[5].XMin = radius - corner_band;
Field[5].XMax = radius + 1e-9;
Field[5].YMin = height - corner_band;
Field[5].YMax = height + 1e-9;

Field[6] = Min;
Field[6].FieldsList = {2, 4, 5};

Background Field = 6;

Mesh.MeshSizeExtendFromBoundary = 0;
Mesh.MeshSizeFromCurvature = 1;
Mesh.MeshSizeFromPoints = 1;
Mesh.Algorithm = 6;

// puck_refined_symmetric_40k.geo
// Symmetric boundary-distance refinement tuned near 40k nodes.
// Keeps moderate surface resolution while using a much smaller bulk size
// so the center elements are less blocky.

lc_shell = 0.050;
lc_bulk = 0.12;
shell_band = 0.08;
trans_band = 0.42;

Include "puck_refined_symmetric.geo";

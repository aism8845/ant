# Static mesh-prep for the 2D RZ nutrient/growth/mechanics case.
# Run from the repo root with:
#   ./ant-opt -i inputs/2d/rz_static_refine_meshprep.i --mesh-only outputs/2d/rz_static_refined.e
#
# The saved mesh can then be reused by inputs/2d/rz_static_run.i.
#
# Notes:
# - `outer_refine_levels` controls the first pass on the nutrient-fed surface.
# - `shell_refine_levels` refines a geometric shell band that extends inward
#   from the nutrient-fed boundaries.
# - `right_shell_xmin` and `top_shell_ymin` set the inner extent of the shell
#   band. Defaults correspond to 5x the original `rz.geo` boundary bands.

mesh_file = rz.msh
outer_boundaries = 'top right'
outer_refine_levels = 2
shell_refine_levels = 1
right_shell_xmin = 1.50
top_shell_ymin = 0.10
enable_neighbor_refinement = true

[Mesh]
  parallel_type = replicated

  [base]
    type = FileMeshGenerator
    file = ${mesh_file}
  []
  [outer_refine]
    type = RefineSidesetGenerator
    input = base
    boundaries = ${outer_boundaries}
    refinement = '${outer_refine_levels} ${outer_refine_levels}'
    boundary_side = 'primary primary'
    enable_neighbor_refinement = ${enable_neighbor_refinement}
  []
  [right_shell]
    type = SubdomainBoundingBoxGenerator
    input = outer_refine
    block_id = 2
    restricted_subdomains = '1'
    bottom_left = '${right_shell_xmin} 0 0'
    top_right = '2.0 0.5 0'
  []
  [top_shell]
    type = SubdomainBoundingBoxGenerator
    input = right_shell
    block_id = 3
    restricted_subdomains = '1 2'
    bottom_left = '0 ${top_shell_ymin} 0'
    top_right = '2.0 0.5 0'
  []
  [shell_refine]
    type = RefineBlockGenerator
    input = top_shell
    block = '2 3'
    refinement = '${shell_refine_levels} ${shell_refine_levels}'
    enable_neighbor_refinement = ${enable_neighbor_refinement}
  []
  [merge_shell_blocks]
    type = RenameBlockGenerator
    input = shell_refine
    old_block = '2 3'
    new_block = '1 1'
  []

  final_generator = merge_shell_blocks
[]

/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "BLI_string.h"
#include "BLI_math_base.h"

#include "DNA_node_types.h"

#include "BKE_material.h"
#include "BKE_mesh.hh"

#include "GEO_mesh_primitive_grid.hh"

#include "node_geometry_util.hh"

namespace blender::nodes::node_geo_mesh_primitive_grid_cc {

/* Pivot positions */
enum {
  GEO_NODE_GRID_PIVOT_CENTER = 0,    /* Center (Default) */
  GEO_NODE_GRID_PIVOT_CORNER_FRONT_LEFT = 1, /* Corner */
};

struct NodeGeometryMeshGrid {
  int pivot;
};

NODE_STORAGE_FUNCS(NodeGeometryMeshGrid)

static EnumPropertyItem pivot_items[] = {
    {GEO_NODE_GRID_PIVOT_CENTER, "CENTER", 0, "Center", "Center pivot (0, 0, 0)"},
    {GEO_NODE_GRID_PIVOT_CORNER_FRONT_LEFT, "CORNER_FRONT_LEFT", 0, "Corner", "Front left corner pivot (0.5, 0.5, 0)"},
    {0, nullptr, 0, nullptr, nullptr},
};

static void node_declare(NodeDeclarationBuilder &b)
{
  b.add_input<decl::Float>("Size X")
      .default_value(1.0f)
      .min(0.0f)
      .subtype(PROP_DISTANCE)
      .description("Side length of the plane in the X direction");
  b.add_input<decl::Float>("Size Y")
      .default_value(1.0f)
      .min(0.0f)
      .subtype(PROP_DISTANCE)
      .description("Side length of the plane in the Y direction");
  b.add_input<decl::Int>("Vertices X")
      .default_value(3)
      .min(2)
      .max(1000)
      .description("Number of vertices in the X direction");
  b.add_input<decl::Int>("Vertices Y")
      .default_value(3)
      .min(2)
      .max(1000)
      .description("Number of vertices in the Y direction");
  b.add_output<decl::Geometry>("Mesh");
  b.add_output<decl::Vector>("UV Map").field_on_all();
}

static void node_init(bNodeTree * /*tree*/, bNode *node)
{
  NodeGeometryMeshGrid *data = MEM_cnew<NodeGeometryMeshGrid>(__func__);
  data->pivot = GEO_NODE_GRID_PIVOT_CENTER;
  node->storage = data;
}

static void node_layout(uiLayout *layout, bContext * /*C*/, PointerRNA *ptr)
{
  uiLayoutSetPropSep(layout, false);
  uiLayoutSetPropDecorate(layout, false);
  uiItemR(layout, ptr, "pivot", UI_ITEM_NONE, nullptr, ICON_NONE);
}

static void node_rna(StructRNA *srna)
{
  RNA_def_node_enum(srna,
                   "pivot",
                   "Pivot",
                   "Position of the pivot point",
                   pivot_items,
                   NOD_storage_enum_accessors(pivot),
                   GEO_NODE_GRID_PIVOT_CENTER);
}

static float2 calculate_pivot_offset(const int pivot, const float2 size)
{
  const float2 half_size = size * 0.5f;

  switch (pivot) {
    case GEO_NODE_GRID_PIVOT_CENTER:
      return float2(0);
    case GEO_NODE_GRID_PIVOT_CORNER_FRONT_LEFT:
      return float2(-half_size.x, -half_size.y);  /* Move pivot to front left corner */
    default:
      return float2(0);
  }
}

static void node_geo_exec(GeoNodeExecParams params)
{
  const float size_x = params.extract_input<float>("Size X");
  const float size_y = params.extract_input<float>("Size Y");
  const int verts_x = params.extract_input<int>("Vertices X");
  const int verts_y = params.extract_input<int>("Vertices Y");
  if (verts_x < 1 || verts_y < 1) {
    params.set_default_remaining_outputs();
    return;
  }

  std::optional<std::string> uv_map_id = params.get_output_anonymous_attribute_id_if_needed(
      "UV Map");
  const NodeGeometryMeshGrid &storage = node_storage(params.node());
  const float2 pivot_offset = calculate_pivot_offset(storage.pivot, float2(size_x, size_y));

  Mesh *mesh = geometry::create_grid_mesh(verts_x, verts_y, size_x, size_y, uv_map_id);

  /* Apply pivot offset */
  MutableSpan<float3> positions = mesh->vert_positions_for_write();
  for (float3 &pos : positions) {
    pos.x -= pivot_offset.x;
    pos.y -= pivot_offset.y;
  }

  BKE_id_material_eval_ensure_default_slot(&mesh->id);

  params.set_output("Mesh", GeometrySet::from_mesh(mesh));
}

static void node_register()
{
  static blender::bke::bNodeType ntype;

  geo_node_type_base(&ntype, GEO_NODE_MESH_PRIMITIVE_GRID, "Grid", NODE_CLASS_GEOMETRY);
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  ntype.draw_buttons = node_layout;
  ntype.initfunc = node_init;
  node_rna(ntype.rna_ext.srna);
  BLI_strncpy(ntype.storagename, "NodeGeometryMeshGrid", sizeof(ntype.storagename));
  ntype.enum_name_legacy = "MESH_PRIMITIVE_GRID";
  blender::bke::node_register_type(&ntype);
}

NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_geo_mesh_primitive_grid_cc

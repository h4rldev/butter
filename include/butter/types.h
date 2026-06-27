#ifndef BUTTER_TYPES_H
#define BUTTER_TYPES_H

#include <butter/internal/types.h>

typedef enum {
  BUTTER_LOG_DEBUG,
  BUTTER_LOG_INFO,
  BUTTER_LOG_WARNING,
  BUTTER_LOG_ERROR,
  BUTTER_LOG_FATAL,
} butter_log_level_t;

typedef enum butter_backend {
  BUTTER_BACKEND_XCB,
  BUTTER_BACKEND_WAYLAND,
} butter_backend_t;

typedef struct butter_surface_info {
  butter_backend_t backend;
  void *handle;
  void *display;
} butter_surface_info_t;

typedef butter_context_t butter_t;
typedef struct butter_frame butter_frame_t;

typedef enum {
  BUTTER_ATTRIB_POSITION_2D, // vec2 (float x, y)
  BUTTER_ATTRIB_POSITION_3D, // vec3
  BUTTER_ATTRIB_UV,          // vec2
  BUTTER_ATTRIB_COLOR,       // vec4 (float r,g,b,a)
  BUTTER_ATTRIB_NORMAL,      // vec3
  BUTTER_ATTRIB_TANGENT,     // vec3
  BUTTER_ATTRIB_MAX
} butter_attribute_type_t;

typedef struct {
  u32 location;
  u32 offset;
  butter_attribute_type_t type;
} butter_attribute_t;

typedef enum {
  BUTTER_TOPOLOGY_POINT_LIST,
  BUTTER_TOPOLOGY_LINE_LIST,
  BUTTER_TOPOLOGY_LINE_STRIP,
  BUTTER_TOPOLOGY_TRIANGLE_LIST,
  BUTTER_TOPOLOGY_TRIANGLE_STRIP,
  BUTTER_TOPOLOGY_TRIANGLE_FAN,
  BUTTER_TOPOLOGY_LINE_LIST_ADJ,
  BUTTER_TOPOLOGY_LINE_STRIP_ADJ,
  BUTTER_TOPOLOGY_TRIANGLE_LIST_ADJ,
  BUTTER_TOPOLOGY_TRIANGLE_STRIP_ADJ,
  BUTTER_TOPOLOGY_MAX
} butter_primitive_topology_t;

typedef enum {
  BUTTER_CULL_NONE,
  BUTTER_CULL_FRONT,
  BUTTER_CULL_BACK,
  BUTTER_CULL_BOTH,
  BUTTER_CULL_MAX
} butter_cull_mode_t;

typedef enum {
  BUTTER_POLYGON_MODE_FILL,
  BUTTER_POLYGON_MODE_LINE,
  BUTTER_POLYGON_MODE_POINT,
  BUTTER_POLYGON_MODE_MAX
} butter_polygon_mode_t;

typedef enum {
  BUTTER_BLEND_NONE,
  BUTTER_BLEND_ALPHA,
  BUTTER_BLEND_ADDITIVE,
  BUTTER_BLEND_PREMULTIPLIED,
  BUTTER_BLEND_MAX
} butter_blend_mode_t;

typedef enum {
  BUTTER_FRONT_FACE_CLOCKWISE,
  BUTTER_FRONT_FACE_COUNTER_CLOCKWISE,
  BUTTER_FRONT_FACE_MAX
} butter_front_face_t;

typedef enum {
  BUTTER_STAGE_VERTEX,
  BUTTER_STAGE_TESSELLATION_CONTROL,
  BUTTER_STAGE_TESSELLATION_EVALUATION,
  BUTTER_STAGE_GEOMETRY,
  BUTTER_STAGE_FRAGMENT,
  BUTTER_STAGE_MAX
} butter_shader_stage_t;

typedef struct {
  butter_shader_stage_t stage;
  const void *code;
  const vk_specialization_info_t *spec;
  const cstr *entry_point;
  u64 code_size;
} butter_shader_t;

typedef struct {
  butter_shader_t *shaders;
  u64 shaders_count;

  const butter_attribute_t *attributes;
  u32 attribute_count;
  u32 vertex_stride;

  const vk_descriptor_set_layout_t *descriptor_set_layouts;
  u32 descriptor_set_layout_count;

  butter_primitive_topology_t topology;
  butter_polygon_mode_t polygon_mode;
  butter_cull_mode_t cull_mode;
  butter_blend_mode_t blend_mode;
  butter_front_face_t front_face;
  b32 depth_test;
  b32 depth_write;
} butter_pipeline_desc_t;

typedef struct {
  vk_pipeline_layout_t layout;
  vk_pipeline_t pipeline;
} butter_pipeline_t;

typedef struct butter_buffer butter_buffer_t;

typedef struct {
  vk_descriptor_set_t set;
  vk_descriptor_set_layout_t layout;
} butter_descriptor_set_t;

typedef struct {
  vk_buffer_t buffer;
  u64 offset;
  void *mapped;
} butter_allocation_t;

typedef struct {
  vk_filter_t mag_filter;
  vk_filter_t min_filter;
  vk_sampler_mipmap_mode_t mipmap_mode;
  vk_sampler_address_mode_t address_mode_u;
  vk_sampler_address_mode_t address_mode_v;
  vk_sampler_address_mode_t address_mode_w;
  f32 max_anisotropy;
  b32 compare_enable;
  vk_compare_op_t compare_op;
} butter_sampler_desc_t;

typedef struct butter_texture butter_texture_t;

typedef struct {
  butter_pipeline_t pipeline;
  vk_buffer_t vertex_buffer;
  u32 vertex_count;
  u32 vertex_offset;
  vk_buffer_t index_buffer;
  u32 index_count;
  vk_index_type_t index_type;
  const butter_descriptor_set_t *descriptor_sets;
  u32 descriptor_set_count;
} butter_draw_cmd_t;

#endif // !BUTTER_TYPES_H

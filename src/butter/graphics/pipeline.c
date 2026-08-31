/***********************************/

#include <htils/basictypes.h>

#include <butter/log.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Get the size of an attribute type.
 * @details Maps the attribute type to the size of the corresponding structure
 *
 * @param type The attribute type.
 *
 * @pre @c type must be a valid attribute type.
 *
 * @return The size of the attribute type.
 */
static u32 attrib_type_size(butter_attribute_type_t type) {
  switch (type) {
  case BUTTER_ATTRIB_POSITION_2D:
    return 8;
  case BUTTER_ATTRIB_POSITION_3D:
    return 12;
  case BUTTER_ATTRIB_UV:
    return 8;
  case BUTTER_ATTRIB_COLOR:
    return 16;
  case BUTTER_ATTRIB_NORMAL:
    return 12;
  case BUTTER_ATTRIB_TANGENT:
    return 12;
  default:
    return 0;
  }
}

//
//
//

/**
 * @brief Get the format of an attribute type.
 * @details Maps the attribute type to the corresponding Vulkan format.
 *
 * @param a The attribute type.
 *
 * @pre @c a must be a valid attribute type.
 *
 * @return The corresponding Vulkan format.
 */
static vk_format_t attr_to_vk_format(butter_attribute_type_t a) {
  switch (a) {
  case BUTTER_ATTRIB_UV:
  case BUTTER_ATTRIB_POSITION_2D:
    return VK_FORMAT_R32G32_SFLOAT;
  case BUTTER_ATTRIB_POSITION_3D:
  case BUTTER_ATTRIB_NORMAL:
  case BUTTER_ATTRIB_TANGENT:
    return VK_FORMAT_R32G32B32_SFLOAT;
  case BUTTER_ATTRIB_COLOR:
    return VK_FORMAT_R32G32B32A32_SFLOAT;
  default:
    return VK_FORMAT_UNDEFINED;
  }
}

//
//
//

/**
 * @brief Convert a primitive topology to Vulkan's.
 * @details Maps the primitive topology to the Vulkan equivalent.
 *
 * @param t The primitive topology.
 *
 * @pre @c t must be a valid primitive topology.
 *
 * @return The corresponding Vulkan enum field.
 */
static vk_primitive_topology_t topology_to_vk(butter_primitive_topology_t t) {
  switch (t) {
  case BUTTER_TOPOLOGY_POINT_LIST:
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  case BUTTER_TOPOLOGY_LINE_LIST:
    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  case BUTTER_TOPOLOGY_LINE_STRIP:
    return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
  case BUTTER_TOPOLOGY_TRIANGLE_LIST:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  case BUTTER_TOPOLOGY_TRIANGLE_STRIP:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  case BUTTER_TOPOLOGY_TRIANGLE_FAN:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
  case BUTTER_TOPOLOGY_LINE_LIST_ADJ:
    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
  case BUTTER_TOPOLOGY_LINE_STRIP_ADJ:
    return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
  case BUTTER_TOPOLOGY_TRIANGLE_LIST_ADJ:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
  case BUTTER_TOPOLOGY_TRIANGLE_STRIP_ADJ:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
  default:
    return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
  }
}

//
//
//

/**
 * @brief Convert a cull mode to Vulkan's.
 * @details Maps the cull mode to the Vulkan equivalent.
 *
 * @param c The cull mode.
 *
 * @pre @c c must be a valid cull mode.
 *
 * @return The corresponding Vulkan enum field.
 */
static vk_cull_mode_t cull_mode_to_vk(butter_cull_mode_t c) {
  switch (c) {
  case BUTTER_CULL_NONE:
    return VK_CULL_MODE_NONE;
  case BUTTER_CULL_FRONT:
    return VK_CULL_MODE_FRONT_BIT;
  case BUTTER_CULL_BACK:
    return VK_CULL_MODE_BACK_BIT;
  case BUTTER_CULL_BOTH:
    return VK_CULL_MODE_FRONT_AND_BACK;
  default:
    return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
  }
}

//
//
//

/**
 * @brief Convert a polygon mode to Vulkan's.
 * @details Maps the polygon mode to the Vulkan equivalent.
 *
 * @param p The polygon mode.
 *
 * @pre @c p must be a valid polygon mode.
 *
 * @return The corresponding Vulkan enum field.
 */
static vk_polygon_mode_t polygon_mode_to_vk(butter_polygon_mode_t p) {
  switch (p) {
  case BUTTER_POLYGON_MODE_FILL:
    return VK_POLYGON_MODE_FILL;
  case BUTTER_POLYGON_MODE_LINE:
    return VK_POLYGON_MODE_LINE;
  case BUTTER_POLYGON_MODE_POINT:
    return VK_POLYGON_MODE_POINT;
  default:
    return VK_POLYGON_MODE_MAX_ENUM;
  }
}

//
//
//

/**
 * @brief Convert a front face to Vulkan's.
 * @details Maps the front face to the Vulkan equivalent.
 *
 * @param f The front face.
 *
 * @pre @c f must be a valid front face.
 *
 * @return The corresponding Vulkan enum field.
 */
static vk_front_face_t front_face_to_vk(butter_front_face_t f) {
  return f == BUTTER_FRONT_FACE_CLOCKWISE ? VK_FRONT_FACE_CLOCKWISE
                                          : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

//
//
//

/**
 * @brief Convert a shader stage to Vulkan's.
 * @details Maps the shader stage to the Vulkan equivalent.
 *
 * @param s The shader stage.
 *
 * @pre @c s must be a valid shader stage.
 *
 * @return The corresponding Vulkan enum field.
 */
static vk_shader_stage_flags_t shader_stage_to_vk(butter_shader_stage_t s) {
  switch (s) {
  case BUTTER_STAGE_VERTEX:
    return VK_SHADER_STAGE_VERTEX_BIT;
  case BUTTER_STAGE_TESSELLATION_CONTROL:
    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
  case BUTTER_STAGE_TESSELLATION_EVALUATION:
    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
  case BUTTER_STAGE_GEOMETRY:
    return VK_SHADER_STAGE_GEOMETRY_BIT;
  case BUTTER_STAGE_FRAGMENT:
    return VK_SHADER_STAGE_FRAGMENT_BIT;
  default:
    butter_log_error("Unknown shader stage: %d", s);
    return VK_SHADER_STAGE_COMPUTE_BIT;
  }
}

//
//
//

/**
 * @brief Map a blend mode to Vulkan's.
 * @details Maps the blend mode to the Vulkan equivalent.
 *
 * @param blend The blend mode.
 *
 * @pre @c blend must be a valid blend mode.
 *
 * @return The corresponding Vulkan struct.
 */
static void map_blend_mode(butter_blend_mode_t blend,
                           vk_pipeline_color_blend_attachment_state_t *out) {
  switch (blend) {
  case BUTTER_BLEND_NONE:
    out->blendEnable = VK_FALSE;
    break;
  case BUTTER_BLEND_ALPHA:
    out->blendEnable = VK_TRUE;
    out->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    out->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    out->colorBlendOp = VK_BLEND_OP_ADD;
    out->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    out->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    out->alphaBlendOp = VK_BLEND_OP_ADD;
    break;
  case BUTTER_BLEND_ADDITIVE:
    out->blendEnable = VK_TRUE;
    out->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    out->dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    out->colorBlendOp = VK_BLEND_OP_ADD;
    out->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    out->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    out->alphaBlendOp = VK_BLEND_OP_ADD;
    break;
  case BUTTER_BLEND_PREMULTIPLIED:
    out->blendEnable = VK_TRUE;
    out->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    out->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    out->colorBlendOp = VK_BLEND_OP_ADD;
    out->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    out->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    out->alphaBlendOp = VK_BLEND_OP_ADD;
    break;
  default:
    butter_log_error("Unknown blend mode: %d", blend);
    out->blendEnable = VK_FALSE;
    break;
  }
}

//
//
//

/**
 * @brief Validate a pipeline descriptor.
 * @details Validates the pipeline descriptor for correctness by checking for
 * inconsistencies.
 *
 * @param desc The pipeline descriptor.
 *
 * @pre @c desc must be a valid pipeline descriptor.
 *
 * @return True if the descriptor is valid, false otherwise.
 */
static b32 butter_validate_pipeline_desc(const butter_pipeline_desc_t *desc) {
  if (!desc || !desc->shaders || desc->shaders_count == 0) {
    butter_log_error("Pipeline must have at least one shader");
    return false;
  }

  u32 stage_count[5] = {0};
  b32 has_vertex = false;
  b32 has_fragment = false;

  for (u32 i = 0; i < desc->shaders_count; i++) {
    butter_shader_t *s = &desc->shaders[i];

    u32 idx = (u32)s->stage;
    if (idx >= 5) {
      butter_log_error("Unknown shader stage: %d", s->stage);
      return false;
    }

    stage_count[idx]++;
    if (stage_count[idx] > 1) {
      butter_log_error("Duplicate shader stage: %d", s->stage);
      return false;
    }

    if (s->stage == BUTTER_STAGE_VERTEX)
      has_vertex = true;
    if (s->stage == BUTTER_STAGE_FRAGMENT)
      has_fragment = true;
  }

  if (!has_vertex) {
    butter_log_error("Pipeline must have a vertex shader");
    return false;
  }

  if (!has_fragment) {
    butter_log_error("Pipeline must have a fragment shader");
    return false;
  }

  b32 has_tcs = stage_count[1] > 0;
  b32 has_tes = stage_count[2] > 0;
  if (has_tcs != has_tes) {
    butter_log_error(
        "Tessellation Control and Evaluation shaders must be used together");
    return false;
  }

  return true;
}

//
//
//

butter_pipeline_desc_t butter_pipeline_desc_default(void) {
  return (butter_pipeline_desc_t){
      .shaders = null,
      .shaders_count = 0,
      .attributes = null,
      .attribute_count = 0,
      .vertex_stride = 0,
      .descriptor_set_layouts = null,
      .descriptor_set_layout_count = 0,
      .topology = BUTTER_TOPOLOGY_TRIANGLE_LIST,
      .polygon_mode = BUTTER_POLYGON_MODE_FILL,
      .cull_mode = BUTTER_CULL_BACK,
      .blend_mode = BUTTER_BLEND_NONE,
      .front_face = BUTTER_FRONT_FACE_CLOCKWISE,
      .depth_test = true,
      .depth_write = true,
  };
}

void butter_pipeline_desc_add_shaders(butter_pipeline_desc_t *desc,
                                      butter_shader_t *shaders,
                                      u64 shader_count) {
  desc->shaders_count = shader_count;
  desc->shaders = shaders;
}

void butter_pipeline_desc_add_descriptor_set_layouts(
    butter_pipeline_desc_t *desc, vk_descriptor_set_layout_t *layouts,
    u32 layout_count) {
  desc->descriptor_set_layouts = layouts;
  desc->descriptor_set_layout_count = layout_count;
}

void butter_pipeline_desc_add_attributes(butter_pipeline_desc_t *desc,
                                         butter_attribute_t *attributes,
                                         u32 attribute_count) {
  desc->attributes = attributes;
  desc->attribute_count = attribute_count;

  u32 stride = 0;
  for (u32 i = 0; i < attribute_count; i++) {
    u32 end = attributes[i].offset + attrib_type_size(attributes[i].type);
    if (end > stride)
      stride = end;
  }
  desc->vertex_stride = stride;
}

void butter_pipeline_desc_set_vertex_stride(butter_pipeline_desc_t *desc,
                                            u32 stride) {
  desc->vertex_stride = stride;
}

butter_pipeline_t butter_create_pipeline(butter_t *butter,
                                         const butter_pipeline_desc_t *desc,
                                         vk_render_pass_t render_pass) {
  b32 uses_descriptors = desc->descriptor_set_layout_count > 0;
  if (desc->shaders_count == 0) {
    butter_log_error("Can't create pipeline with no shaders");
    return (butter_pipeline_t){0};
  }

  if (desc->shaders_count >= BUTTER_STAGE_MAX) {
    butter_log_error("Can't create pipeline with more than 5 shaders, only 1 "
                     "shader of each type are allowed.");
    return (butter_pipeline_t){0};
  }

  if (!butter_validate_pipeline_desc(desc))
    return (butter_pipeline_t){0};

  for (u32 i = 0; i < desc->shaders_count; i++) {
    if (!desc->shaders[i].entry_point) {
      butter_log_warning(
          "Shader at index %d has no entry point (defaulting to 'main')", i);
    }
  }

  vk_shader_module_t *modules = arena_alloc_zeroed(
      butter->arena, vk_shader_module_t, desc->shaders_count);
  if (!modules) {
    butter_log_fatal("Could not allocate shader modules");
    goto fail;
  }

  for (u32 i = 0; i < desc->shaders_count; i++) {
    vk_shader_module_create_info_t info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = desc->shaders[i].code_size,
        .pCode = (const u32 *)desc->shaders[i].code,
        .pNext = null,
    };

    if (vkCreateShaderModule(butter->device, &info, null, &modules[i]) !=
        VK_SUCCESS) {
      butter_log_error("Could not create shader module at index %d", i);
      goto fail;
    }
  }

  vk_pipeline_shader_stage_create_info_t *shader_stages =
      arena_alloc_zeroed(butter->arena, vk_pipeline_shader_stage_create_info_t,
                         desc->shaders_count);
  if (!shader_stages) {
    butter_log_fatal("Could not allocate shader stages");
    goto fail;
  }

  for (u32 i = 0; i < desc->shaders_count; i++) {
    vk_shader_stage_flags_t current_flag =
        shader_stage_to_vk(desc->shaders[i].stage);
    if (current_flag == VK_SHADER_STAGE_COMPUTE_BIT) {
      butter_log_error("Could not create shader stage at index %d", i);
      goto fail;
    }

    shader_stages[i].sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[i].stage = current_flag;
    shader_stages[i].pName =
        desc->shaders[i].entry_point ? desc->shaders[i].entry_point : "main";
    shader_stages[i].pSpecializationInfo =
        desc->shaders[i].spec ? desc->shaders[i].spec : null;
    shader_stages[i].module = modules[i];
  }

  if (desc->attribute_count > 0 && desc->vertex_stride == 0) {
    butter_log_error("Vertex stride must be > 0 when attributes are present");
    goto fail;
  }

  vk_vertex_input_binding_description_t binding_description = {0};
  binding_description.stride = desc->vertex_stride;
  binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  vk_vertex_input_attribute_description_t *attrs =
      arena_alloc_zeroed(butter->arena, vk_vertex_input_attribute_description_t,
                         desc->attribute_count);
  for (u32 i = 0; i < desc->attribute_count; i++) {
    attrs[i].location = desc->attributes[i].location;
    attrs[i].binding = 0;
    attrs[i].format = attr_to_vk_format(desc->attributes[i].type);
    attrs[i].offset = desc->attributes[i].offset;
  }

  vk_pipeline_vertex_input_state_create_info_t vertex_input = {0};
  vertex_input.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input.vertexBindingDescriptionCount = 1;
  vertex_input.pVertexBindingDescriptions = &binding_description;
  vertex_input.vertexAttributeDescriptionCount = desc->attribute_count;
  vertex_input.pVertexAttributeDescriptions = attrs;

  vk_pipeline_input_assembly_state_create_info_t input_assembly = {0};
  input_assembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = topology_to_vk(desc->topology);
  input_assembly.primitiveRestartEnable = VK_FALSE;

  vk_pipeline_viewport_state_create_info_t viewport = {0};
  viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport.viewportCount = 1;
  viewport.scissorCount = 1;

  vk_pipeline_rasterization_state_create_info_t rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = polygon_mode_to_vk(desc->polygon_mode);
  rasterizer.cullMode = cull_mode_to_vk(desc->cull_mode);
  rasterizer.frontFace = front_face_to_vk(desc->front_face);
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.lineWidth = 1.0f;

  vk_pipeline_multisample_state_create_info_t multisample = {0};
  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample.sampleShadingEnable = VK_FALSE;

  vk_pipeline_color_blend_attachment_state_t color_blend_attachments = {0};
  color_blend_attachments.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachments.blendEnable = VK_FALSE;

  map_blend_mode(desc->blend_mode, &color_blend_attachments);

  vk_pipeline_color_blend_state_create_info_t color_blend = {0};
  color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blend.attachmentCount = 1;
  color_blend.pAttachments = &color_blend_attachments;

  vk_dynamic_state_t dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  vk_pipeline_dynamic_state_create_info_t dynamic_state = {0};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount = 2;
  dynamic_state.pDynamicStates = dynamic_states;

  vk_pipeline_depth_stencil_state_create_info_t depth_stencil = {0};
  depth_stencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil.depthTestEnable = desc->depth_test;
  depth_stencil.depthWriteEnable = desc->depth_write;
  depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depth_stencil.depthBoundsTestEnable = VK_FALSE;
  depth_stencil.stencilTestEnable = VK_FALSE;

  vk_pipeline_layout_create_info_t layout_info = {0};
  layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  layout_info.setLayoutCount = desc->descriptor_set_layout_count;
  layout_info.pSetLayouts = desc->descriptor_set_layouts;

  vk_pipeline_layout_t layout;
  if (vkCreatePipelineLayout(butter->device, &layout_info, null, &layout) !=
      VK_SUCCESS) {
    butter_log_error("Could not create pipeline layout");
    goto fail;
  }

  vk_graphics_pipeline_create_info_t pipeline_info = {0};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = desc->shaders_count;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pDepthStencilState = &depth_stencil;
  pipeline_info.pVertexInputState = &vertex_input;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisample;
  pipeline_info.pColorBlendState = &color_blend;
  pipeline_info.pDynamicState = &dynamic_state;
  pipeline_info.layout = layout;
  pipeline_info.renderPass = render_pass;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_info.basePipelineIndex = -1;

  vk_pipeline_t pipeline;
  if (vkCreateGraphicsPipelines(butter->device, butter->pipeline_cache, 1,
                                &pipeline_info, null,
                                &pipeline) != VK_SUCCESS) {
    butter_log_error("Could not create graphics pipeline");
    vkDestroyPipelineLayout(butter->device, layout, null);
    goto fail;
  }

  for (u32 i = 0; i < desc->shaders_count; i++)
    vkDestroyShaderModule(butter->device, modules[i], null);

  return (butter_pipeline_t){
      .pipeline = pipeline,
      .layout = layout,
      .uses_descriptors = uses_descriptors,
  };

fail:
  for (u32 i = 0; i < desc->shaders_count; i++)
    if (modules[i])
      vkDestroyShaderModule(butter->device, modules[i], null);
  return (butter_pipeline_t){0};
}

void butter_destroy_pipeline(butter_t *butter, butter_pipeline_t *pipeline) {
  butter_log_debug("Destroying pipeline");

  if (!butter || !pipeline)
    return;

  if (pipeline->layout) {
    butter_log_debug("Destroying pipeline layout");
    vkDestroyPipelineLayout(butter->device, pipeline->layout, null);
  }

  if (pipeline->pipeline) {
    butter_log_debug("Destroying graphics pipeline");
    vkDestroyPipeline(butter->device, pipeline->pipeline, null);
  }
}

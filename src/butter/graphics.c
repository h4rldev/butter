#include <htils/basictypes.h>

#include <butter/graphics.h>
#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/texture.h>
#include <butter/types.h>
#include <vulkan/vulkan_core.h>

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

static i32 find_memory_type(vk_physical_device_t physical_device,
                            u32 memory_type_bits,
                            vk_memory_property_flags_t required_properties) {
  vk_physical_device_memory_properties_t mem_props;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

  for (u32 i = 0; i < mem_props.memoryTypeCount; i++) {
    if ((memory_type_bits & (1 << i)) &&
        (mem_props.memoryTypes[i].propertyFlags & required_properties) ==
            required_properties) {
      return i;
    }
  }
  butter_log_fatal("Failed to find suitable memory type");
  return -1;
}

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

void map_blend_mode(butter_blend_mode_t blend,
                    vk_pipeline_color_blend_attachment_state_t *out) {
  if (blend == BUTTER_BLEND_NONE) {
    out->blendEnable = VK_FALSE;
    return;
  } else {
    out->blendEnable = VK_TRUE;
  }

  switch (blend) {
  case BUTTER_BLEND_ALPHA:
    out->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    out->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    out->colorBlendOp = VK_BLEND_OP_ADD;
    out->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    out->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    out->alphaBlendOp = VK_BLEND_OP_ADD;
    break;
  case BUTTER_BLEND_ADDITIVE:
    out->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    out->dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    out->colorBlendOp = VK_BLEND_OP_ADD;
    out->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    out->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    out->alphaBlendOp = VK_BLEND_OP_ADD;
    break;
  case BUTTER_BLEND_PREMULTIPLIED:
    out->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    out->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    out->colorBlendOp = VK_BLEND_OP_ADD;
    out->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    out->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    out->alphaBlendOp = VK_BLEND_OP_ADD;
  default:
    return;
  };
}

static vk_front_face_t front_face_to_vk(butter_front_face_t f) {
  return f == BUTTER_FRONT_FACE_CLOCKWISE ? VK_FRONT_FACE_CLOCKWISE
                                          : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

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
      return (butter_pipeline_t){0};
    }
  }

  vk_pipeline_shader_stage_create_info_t *shader_stages =
      arena_alloc_zeroed(butter->arena, vk_pipeline_shader_stage_create_info_t,
                         desc->shaders_count);
  for (u32 i = 0; i < desc->shaders_count; i++) {
    vk_shader_stage_flags_t current_flag =
        shader_stage_to_vk(desc->shaders[i].stage);
    if (current_flag == VK_SHADER_STAGE_COMPUTE_BIT) {
      butter_log_error("Could not create shader stage at index %d", i);
      return (butter_pipeline_t){0};
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
    return (butter_pipeline_t){0};
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
    return (butter_pipeline_t){0};
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
    for (u32 i = 0; i < desc->shaders_count; i++)
      vkDestroyShaderModule(butter->device, modules[i], null);

    return (butter_pipeline_t){0};
  }

  for (u32 i = 0; i < desc->shaders_count; i++)
    vkDestroyShaderModule(butter->device, modules[i], null);

  return (butter_pipeline_t){
      .pipeline = pipeline,
      .layout = layout,
      .uses_descriptors = uses_descriptors,
  };
}

void butter_destroy_pipeline(butter_t *butter, butter_pipeline_t *pipeline) {
  butter_log_debug("Destroying pipeline");

  if (!butter && !pipeline)
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

butter_buffer_t butter_create_buffer(butter_t *butter, u64 size,
                                     vk_buffer_usage_flags_t usage,
                                     b32 host_visible) {
  vk_result_t res;

  vk_buffer_create_info_t buffer_info = {0};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  vk_buffer_t buffer;
  if ((res = vkCreateBuffer(butter->device, &buffer_info, null, &buffer)) !=
      VK_SUCCESS) {
    butter_log_error("Could not create buffer: %d", res);
    return (butter_buffer_t){0};
  }

  vk_memory_requirements_t mem_reqs;
  vkGetBufferMemoryRequirements(butter->device, buffer, &mem_reqs);

  i32 memory_type =
      find_memory_type(butter->physical_device, mem_reqs.memoryTypeBits,
                       host_visible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                                    : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memory_type == -1 && !host_visible)
    memory_type =
        find_memory_type(butter->physical_device, mem_reqs.memoryTypeBits,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (memory_type == -1) {
    butter_log_fatal("Could not find suitable memory type");
    return (butter_buffer_t){0};
  }

  vk_memory_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = memory_type;

  vk_device_memory_t memory;
  if ((res = vkAllocateMemory(butter->device, &alloc_info, null, &memory)) !=
      VK_SUCCESS) {
    butter_log_fatal("Could not allocate buffer memory: %d", res);
    return (butter_buffer_t){0};
  }

  void *mapped;
  if ((res = vkMapMemory(butter->device, memory, 0, mem_reqs.size, 0,
                         &mapped)) != VK_SUCCESS) {
    butter_log_fatal("Could not map buffer memory: %d", res);
    return (butter_buffer_t){0};
  }

  if ((res = vkBindBufferMemory(butter->device, buffer, memory, 0)) !=
      VK_SUCCESS) {
    butter_log_fatal("Could not bind buffer memory: %d", res);
    return (butter_buffer_t){0};
  }

  butter_buffer_t butter_buffer = {0};
  butter_buffer.handle = buffer;
  butter_buffer.memory = memory;
  butter_buffer.size = size;
  butter_buffer.mapped = mapped;

  return butter_buffer;
}

void butter_destroy_buffer(butter_t *butter, butter_buffer_t *buffer) {
  if (!buffer || buffer->handle == VK_NULL_HANDLE)
    return;

  if (buffer->mapped) {
    vkUnmapMemory(butter->device, buffer->memory);
    buffer->mapped = null;
  }
  if (buffer->memory) {
    vkFreeMemory(butter->device, buffer->memory, NULL);
    buffer->memory = VK_NULL_HANDLE;
  }
  if (buffer->handle) {
    vkDestroyBuffer(butter->device, buffer->handle, NULL);
    buffer->handle = VK_NULL_HANDLE;
  }
}

vk_descriptor_pool_t
butter_create_descriptor_pool(butter_t *butter, u32 max_sets,
                              const vk_descriptor_pool_size_t *pool_sizes,
                              u32 pool_size_count) {
  vk_descriptor_pool_create_info_t pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.maxSets = max_sets;
  pool_info.poolSizeCount = pool_size_count;
  pool_info.pPoolSizes = pool_sizes;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  vk_descriptor_pool_t pool;
  if (vkCreateDescriptorPool(butter->device, &pool_info, null, &pool) !=
      VK_SUCCESS) {
    butter_log_error("Could not create descriptor pool");
    return VK_NULL_HANDLE;
  }

  return pool;
}

void butter_destroy_descriptor_pool(butter_t *butter,
                                    vk_descriptor_pool_t pool) {
  if (pool)
    vkDestroyDescriptorPool(butter->device, pool, null);
}

vk_descriptor_set_layout_t butter_create_descriptor_set_layout(
    butter_t *butter, vk_descriptor_set_layout_binding_t *bindings,
    u32 binding_count) {

  vk_descriptor_set_layout_create_info_t layout_info = {0};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = binding_count;
  layout_info.pBindings = bindings;

  vk_descriptor_set_layout_t layout;
  vk_result_t res;
  if ((res = vkCreateDescriptorSetLayout(butter->device, &layout_info, null,
                                         &layout)) != VK_SUCCESS) {
    butter_log_error("Could not create descriptor set layout: %d", res);
    return VK_NULL_HANDLE;
  }

  return layout;
}

butter_descriptor_set_t
butter_allocate_descriptor_set(butter_t *butter, vk_descriptor_pool_t pool,
                               vk_descriptor_set_layout_t layout) {
  butter_descriptor_set_t set = {0};
  set.layout = layout;

  vk_descriptor_set_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &layout;

  if (vkAllocateDescriptorSets(butter->device, &alloc_info, &set.set) !=
      VK_SUCCESS) {
    butter_log_error("Could not allocate descriptor set");
    return (butter_descriptor_set_t){0};
  }

  return set;
}

void butter_update_descriptor_buffer(butter_t *butter,
                                     butter_descriptor_set_t *set, u32 binding,
                                     vk_buffer_t buffer, u64 offset, u64 size) {
  vk_descriptor_buffer_info_t buffer_info = {0};
  buffer_info.buffer = buffer;
  buffer_info.offset = offset;
  buffer_info.range = size;

  vk_write_descriptor_set_t descriptor_write = {0};
  descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_write.dstSet = set->set;
  descriptor_write.dstBinding = binding;
  descriptor_write.dstArrayElement = 0;
  descriptor_write.descriptorCount = 1;
  descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_write.pBufferInfo = &buffer_info;

  vkUpdateDescriptorSets(butter->device, 1, &descriptor_write, 0, null);
}

void butter_update_descriptor_image(butter_t *butter,
                                    butter_descriptor_set_t *set, u32 binding,
                                    vk_image_view_t image_view,
                                    vk_sampler_t sampler) {
  vk_descriptor_image_info_t image_info = {0};
  image_info.imageView = image_view;
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.sampler = sampler;

  vk_write_descriptor_set_t descriptor_write = {0};
  descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_write.dstSet = set->set;
  descriptor_write.dstBinding = binding;
  descriptor_write.dstArrayElement = 0;
  descriptor_write.descriptorCount = 1;
  descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptor_write.pImageInfo = &image_info;

  vkUpdateDescriptorSets(butter->device, 1, &descriptor_write, 0, null);
}

butter_sampler_desc_t butter_sampler_desc_linear_clamp(void) {
  return (butter_sampler_desc_t){
      .mag_filter = VK_FILTER_LINEAR,
      .min_filter = VK_FILTER_LINEAR,
      .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .max_anisotropy = 0.0f,
      .compare_enable = false,
      .compare_op = VK_COMPARE_OP_NEVER,
  };
}

butter_sampler_desc_t butter_sampler_desc_linear_repeat(void) {
  return (butter_sampler_desc_t){
      .mag_filter = VK_FILTER_LINEAR,
      .min_filter = VK_FILTER_LINEAR,
      .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .max_anisotropy = 0.0f,
      .compare_enable = false,
      .compare_op = VK_COMPARE_OP_NEVER,
  };
}

butter_sampler_desc_t butter_sampler_desc_nearest_clamp(void) {
  return (butter_sampler_desc_t){
      .mag_filter = VK_FILTER_NEAREST,
      .min_filter = VK_FILTER_NEAREST,
      .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .max_anisotropy = 0.0f,
      .compare_enable = false,
      .compare_op = VK_COMPARE_OP_NEVER,
  };
}

butter_sampler_desc_t butter_sampler_desc_anisotropic(f32 max_anisotropy) {
  return (butter_sampler_desc_t){
      .mag_filter = VK_FILTER_LINEAR,
      .min_filter = VK_FILTER_LINEAR,
      .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .max_anisotropy = max_anisotropy,
      .compare_enable = false,
      .compare_op = VK_COMPARE_OP_NEVER,
  };
}

vk_sampler_t butter_create_sampler(butter_t *butter,
                                   const butter_sampler_desc_t *desc) {
  vk_result_t res;

  vk_sampler_create_info_t sampler_info = {0};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = desc->mag_filter;
  sampler_info.minFilter = desc->min_filter;
  sampler_info.mipmapMode = desc->mipmap_mode;
  sampler_info.addressModeU = desc->address_mode_u;
  sampler_info.addressModeV = desc->address_mode_v;
  sampler_info.addressModeW = desc->address_mode_w;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = VK_LOD_CLAMP_NONE;
  sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  sampler_info.anisotropyEnable = desc->max_anisotropy > 1.0f;
  sampler_info.maxAnisotropy = desc->max_anisotropy;
  sampler_info.compareEnable = desc->compare_enable;
  sampler_info.compareOp = desc->compare_op;

  vk_sampler_t sampler;
  if ((res = vkCreateSampler(butter->device, &sampler_info, null, &sampler)) !=
      VK_SUCCESS) {
    butter_log_error("Could not create sampler: %d", res);
    return VK_NULL_HANDLE;
  }

  return sampler;
}

void butter_destroy_sampler(butter_t *butter, vk_sampler_t sampler) {
  if (!butter)
    return;

  if (sampler)
    vkDestroySampler(butter->device, sampler, null);
}

void butter_submit_draws(butter_t *butter, const butter_draw_cmd_t *cmds,
                         u32 count) {
  if (!butter || !cmds || count == 0)
    return;

  vk_command_buffer_t cmd = butter->cmds[butter->frame_index];
  if (!cmd) {
    butter_log_error("No command buffer available");
    return;
  }

  vk_viewport_t viewport = {0};
  viewport.x = 0.0f;
  viewport.y = (f32)butter->extent.height;
  viewport.width = (f32)butter->extent.width;
  viewport.height = -(f32)butter->extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(cmd, 0, 1, &viewport);

  for (u32 i = 0; i < count; i++) {
    const butter_draw_cmd_t *draw = &cmds[i];

    if (draw->scissor_enabled)
      vkCmdSetScissor(cmd, 0, 1, &draw->scissor);
    else {
      vk_rect2d_t scissor = {
          .offset = {0, 0},
          .extent = butter->extent,
      };
      vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      draw->pipeline.pipeline);
    if (draw->vertex_buffer)
      vkCmdBindVertexBuffers(cmd, 0, 1, &draw->vertex_buffer,
                             &draw->vertex_offset);

    if (draw->pipeline.uses_descriptors) {
      if (draw->descriptor_sets && draw->descriptor_set_count > 0) {
        vk_descriptor_set_t sets[draw->descriptor_set_count];
        for (u32 j = 0; j < draw->descriptor_set_count; j++) {
          sets[j] = draw->descriptor_sets[j].set;
        }

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                draw->pipeline.layout, 0,
                                draw->descriptor_set_count, sets, 0, null);
      } else {
        butter_texture_t *tex = null;
        if (draw->texture_id != 0)
          tex = butter_texture_get(butter, draw->texture_id);

        if (!tex || !butter_texture_is_ready(tex))
          tex = butter_texture_get(butter, 0);

        if (tex)
          vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                  draw->pipeline.layout, 0, 1,
                                  &tex->descriptor_set.set, 0, null);
      }
    }

    if (draw->index_buffer && draw->index_count > 0) {

      vkCmdBindIndexBuffer(cmd, draw->index_buffer, draw->index_offset,
                           draw->index_type);
      vkCmdDrawIndexed(cmd, draw->index_count, 1, 0, 0, 0);
    } else
      vkCmdDraw(cmd, draw->vertex_count, 1, 0, 0);
  }
}

butter_allocation_t butter_alloc_vertices(butter_t *butter, u32 vertex_count,
                                          u32 stride) {
  u32 frame_index = butter->frame_index;
  butter_allocation_t allocation = {0};

  u64 size_needed = (u64)vertex_count * stride;

  if (butter->dynamic_vbo_offset + size_needed > butter->dynamic_vbo_size) {
    butter_log_error("Dynamic buffer overflow");
    return allocation;
  }

  butter_buffer_t *buffer = &butter->dynamic_vbos[frame_index];
  allocation.buffer = buffer->handle;
  allocation.offset = butter->dynamic_vbo_offset;
  allocation.mapped = (u8 *)buffer->mapped + butter->dynamic_vbo_offset;

  butter->dynamic_vbo_offset += size_needed;

  return allocation;
}

butter_allocation_t butter_alloc_indices(butter_t *butter, u32 index_count,
                                         vk_index_type_t index_type) {
  u32 stride = 0;
  switch (index_type) {
  case VK_INDEX_TYPE_UINT16:
    stride = sizeof(u16);
    break;
  case VK_INDEX_TYPE_UINT32:
    stride = sizeof(u32);
    break;
  case VK_INDEX_TYPE_UINT8:
    stride = sizeof(u8);
    break;
  default:
    butter_log_error("Unknown index type: %d", index_type);
    return (butter_allocation_t){0};
  }

  return butter_alloc_vertices(butter, index_count, stride);
}

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/types.h>

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
    shader_stages[i].pNext = null;
  }

  if (desc->attribute_count > 0 && desc->vertex_stride == 0) {
    butter_log_error("Vertex stride must be > 0 when attributes are present");
    return (butter_pipeline_t){0};
  }

  vk_vertex_input_binding_description_t binding_description = {
      .binding = 0,
      .stride = desc->vertex_stride,
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };

  vk_vertex_input_attribute_description_t *attrs =
      arena_alloc_zeroed(butter->arena, vk_vertex_input_attribute_description_t,
                         desc->attribute_count);
  for (u32 i = 0; i < desc->attribute_count; i++) {
    attrs[i].location = desc->attributes[i].location;
    attrs[i].binding = 0;
    attrs[i].format = attr_to_vk_format(desc->attributes[i].type);
    attrs[i].offset = desc->attributes[i].offset;
  }

  vk_pipeline_vertex_input_state_create_info_t vertex_input = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &binding_description,
      .vertexAttributeDescriptionCount = desc->attribute_count,
      .pVertexAttributeDescriptions = attrs,
      .pNext = null,
  };

  vk_pipeline_input_assembly_state_create_info_t input_assembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = topology_to_vk(desc->topology),
      .primitiveRestartEnable = VK_FALSE,
      .pNext = null,
  };

  vk_pipeline_viewport_state_create_info_t viewport = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
      .pNext = null,
  };

  vk_pipeline_rasterization_state_create_info_t rasterizer = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = polygon_mode_to_vk(desc->polygon_mode),
      .cullMode = cull_mode_to_vk(desc->cull_mode),
      .frontFace = front_face_to_vk(desc->front_face),
      .depthBiasEnable = VK_FALSE,
      .lineWidth = 1.0f,
      .pNext = null,
  };

  vk_pipeline_multisample_state_create_info_t multisample = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .pNext = null,
  };

  vk_pipeline_color_blend_attachment_state_t color_blend_attachments = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_FALSE,
  };
  map_blend_mode(desc->blend_mode, &color_blend_attachments);

  vk_pipeline_color_blend_state_create_info_t color_blend = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachments,
      .pNext = null,
  };

  vk_dynamic_state_t dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };

  vk_pipeline_dynamic_state_create_info_t dynamic_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      // Swapchain needs recreation.
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states,
      .pNext = null,
  };

  vk_pipeline_depth_stencil_state_create_info_t depth_stencil = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = desc->depth_test,
      .depthWriteEnable = desc->depth_write,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
      .pNext = null,
  };

  vk_pipeline_layout_create_info_t layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = desc->descriptor_set_layout_count,
      .pSetLayouts = desc->descriptor_set_layouts,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = null,
      .pNext = null,
  };

  vk_pipeline_layout_t layout;
  if (vkCreatePipelineLayout(butter->device, &layout_info, null, &layout) !=
      VK_SUCCESS) {
    butter_log_error("Could not create pipeline layout");
    return (butter_pipeline_t){0};
  }

  vk_graphics_pipeline_create_info_t pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = desc->shaders_count,
      .pStages = shader_stages,
      .pDepthStencilState = &depth_stencil,
      .pVertexInputState = &vertex_input,
      .pInputAssemblyState = &input_assembly,
      .pViewportState = &viewport,
      .pRasterizationState = &rasterizer,
      .pMultisampleState = &multisample,
      .pColorBlendState = &color_blend,
      .pDynamicState = &dynamic_state,
      .layout = layout,
      .renderPass = render_pass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = -1,
      .pNext = null,
  };

  vk_pipeline_t pipeline;
  if (vkCreateGraphicsPipelines(butter->device, VK_NULL_HANDLE, 1,
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
  vk_buffer_create_info_t buffer_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .pNext = null,
  };

  vk_buffer_t buffer;
  if (vkCreateBuffer(butter->device, &buffer_info, null, &buffer) !=
      VK_SUCCESS) {
    butter_log_error("Could not create buffer");
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

  vk_memory_allocate_info_t alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = mem_reqs.size,
      .memoryTypeIndex = memory_type,
  };

  vk_device_memory_t memory;
  if (vkAllocateMemory(butter->device, &alloc_info, null, &memory) !=
      VK_SUCCESS) {
    butter_log_fatal("Could not allocate buffer memory");
    return (butter_buffer_t){0};
  }

  void *mapped;
  if (vkMapMemory(butter->device, memory, 0, mem_reqs.size, 0, &mapped) !=
      VK_SUCCESS) {
    butter_log_fatal("Could not map buffer memory");
    return (butter_buffer_t){0};
  }

  if (vkBindBufferMemory(butter->device, buffer, memory, 0) != VK_SUCCESS) {
    butter_log_fatal("Could not bind buffer memory");
    return (butter_buffer_t){0};
  }

  return (butter_buffer_t){
      .handle = buffer,
      .memory = memory,
      .size = size,
      .mapped = mapped,
  };
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
  vk_descriptor_pool_create_info_t pool_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = max_sets,
      .poolSizeCount = pool_size_count,
      .pPoolSizes = pool_sizes,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .pNext = null,
  };

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

  vk_descriptor_set_layout_create_info_t layout_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = null,
      .flags = 0,
      .bindingCount = binding_count,
      .pBindings = bindings,
  };

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

  vk_descriptor_set_allocate_info_t alloc_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &layout,
      .pNext = null,
  };

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
  vk_descriptor_buffer_info_t buffer_info = {
      .buffer = buffer,
      .offset = offset,
      .range = size,
  };

  vk_write_descriptor_set_t descriptor_write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set->set,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .pBufferInfo = &buffer_info,
  };

  vkUpdateDescriptorSets(butter->device, 1, &descriptor_write, 0, null);
}

void butter_update_descriptor_image(butter_t *butter,
                                    butter_descriptor_set_t *set, u32 binding,
                                    vk_image_view_t image_view,
                                    vk_sampler_t sampler) {
  vk_descriptor_image_info_t image_info = {
      .imageView = image_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .sampler = sampler,
  };

  vk_write_descriptor_set_t descriptor_write = {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .dstSet = set->set,
      .dstBinding = binding,
      .dstArrayElement = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImageInfo = &image_info,
  };

  vkUpdateDescriptorSets(butter->device, 1, &descriptor_write, 0, null);
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

  vk_viewport_t viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = (f32)butter->extent.width,
      .height = (f32)butter->extent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  vkCmdSetViewport(cmd, 0, 1, &viewport);

  vk_rect2d_t scissor = {
      .offset = {0, 0},
      .extent = butter->extent,
  };

  vkCmdSetScissor(cmd, 0, 1, &scissor);

  for (u32 i = 0; i < count; i++) {
    const butter_draw_cmd_t *draw = &cmds[i];

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      draw->pipeline.pipeline);
    if (draw->vertex_buffer) {
      vk_device_size_t offset = draw->vertex_offset;
      vkCmdBindVertexBuffers(cmd, 0, 1, &draw->vertex_buffer, &offset);
    }

    if (draw->index_buffer)
      vkCmdBindIndexBuffer(cmd, draw->index_buffer, 0, draw->index_type);

    if (draw->descriptor_sets && draw->descriptor_set_count > 0) {
      vk_descriptor_set_t sets[draw->descriptor_set_count];
      for (u32 j = 0; j < draw->descriptor_set_count; j++) {
        sets[j] = draw->descriptor_sets[j].set;
      }

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              draw->pipeline.layout, 0,
                              draw->descriptor_set_count, sets, 0, null);
    }

    if (draw->index_buffer && draw->index_count > 0)
      vkCmdDrawIndexed(cmd, draw->index_count, 1, 0, 0, 0);
    else
      vkCmdDraw(cmd, draw->vertex_count, 1, 0, 0);
  }
}

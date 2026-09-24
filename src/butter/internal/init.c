/***********************************/

#include <string.h>

#include <htils/basictypes.h>
#include <htils/file.h>
#include <htils/string.h>

#include <vulkan/vulkan.h>

#include <butter/graphics.h>
#include <butter/texture.h>

#include <butter/render/aa.h>
#include <butter/render/thread.h>

#include <butter/internal/cache.h>
#include <butter/internal/check.h>
#include <butter/internal/device.h>
#include <butter/internal/get.h>
#include <butter/internal/init.h>
#include <butter/internal/stats.h>
#include <butter/internal/swapchain.h>
#include <butter/internal/target.h>
#include <butter/internal/texture.h>
#include <butter/internal/types.h>

#ifdef BUTTER_X11
#include <vulkan/vulkan_xcb.h>
#include <xcb/xcb.h>
#endif

#ifdef BUTTER_WAYLAND
#include <vulkan/vulkan_wayland.h>
#include <wayland-client.h>
#endif

#include <vulkan/vulkan_core.h>

#include <butter/log.h>

#ifndef BUTTER_DYNAMIC_VBO_MIN
#define BUTTER_DYNAMIC_VBO_MIN (KiB(256))
#endif

#ifndef BUTTER_DYNAMIC_IBO_MIN
#define BUTTER_DYNAMIC_IBO_MIN (KiB(64))
#endif

/***********************************/

/**
 * @brief Create a platform surface.
 * @details Creates the surface for the given backend (Wayland or X11) using
 * the handles from the surface info.
 *
 * @param instance The Vulkan instance.
 * @param info The surface info.
 *
 * @pre
 * - @c instance must be a valid Vulkan instance.
 * - @c info must be a valid surface info.
 *
 * @return The created surface, or VK_NULL_HANDLE on failure.
 */
static vk_surface_khr_t
create_platform_surface(vk_instance_t instance,
                        const butter_surface_info_t *info) {
  vk_surface_khr_t surface = VK_NULL_HANDLE;
  vk_result_t res;

  switch (info->backend) {
  case BUTTER_BACKEND_XCB: {
#ifdef BUTTER_X11
    vk_xcb_surface_create_info_khr_t xcb_info = {0};
    xcb_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_info.connection = (xcb_connection_t *)info->display;
    xcb_info.window = (xcb_window_t)(uintptr_t)info->handle;

    if ((res = vkCreateXcbSurfaceKHR(instance, &xcb_info, null, &surface)) !=
        VK_SUCCESS)
      butter_log_error("Could not create xcb surface");
#else
    butter_log_fatal("Butter was not compiled with X11 support");
    return VK_NULL_HANDLE;
#endif
  } break;
  case BUTTER_BACKEND_WAYLAND: {
#ifdef BUTTER_WAYLAND
    vk_wayland_surface_create_info_khr_t wayland_info = {0};
    wayland_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    wayland_info.display = (struct wl_display *)info->display;
    wayland_info.surface = (struct wl_surface *)info->handle;

    if ((res = vkCreateWaylandSurfaceKHR(instance, &wayland_info, null,
                                         &surface)) != VK_SUCCESS)
      butter_log_error("Could not create wayland surface");
#else
    butter_log_fatal("Butter was not compiled with Wayland support");
    return VK_NULL_HANDLE;
#endif
  } break;
  }

  return surface;
}

//
//
//

/**
 * @brief Initialize the texture registry.
 * @details Initializes the texture registry with a default texture, and
 * simply allocates the shader registry.
 *
 * @param context The butter context.
 * @param arena The arena to allocate the registry from.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c arena must be a valid arena.
 *
 * @return true on success, false on error.
 */
static b32 butter_init_textures(butter_context_t *context, arena_t *arena) {
  if (mtx_init(&context->texture_descriptor_mutex, mtx_plain) != thrd_success) {
    butter_log_fatal("Failed to initialize texture descriptor mutex");
    return false;
  }

  if (!context->shader_registry) {
    context->shader_registry =
        arena_alloc_zeroed(arena, struct butter_shader_registry, 1);
    context->shader_registry->capacity = 0;
  }

  context->texture_registry.capacity = BUTTER_TEXTURE_REGISTRY_INITIAL;
  context->texture_registry.entries =
      arena_alloc_zeroed(context->arena, struct butter_texture_registry_entry,
                         BUTTER_TEXTURE_REGISTRY_INITIAL);

  context->texture_registry.count = 0;
  context->texture_registry.next_id = 1;

  vk_descriptor_set_layout_binding_t binding = {0};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  vk_descriptor_set_layout_create_flags_t layout_flags =
      (context->available_vulkan_features & BUTTER_FEATURE_PUSH_DESCRIPTORS)
          ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT
          : 0;

  context->texture_descriptor_set_layout =
      butter_create_descriptor_set_layout(context, &binding, 1, layout_flags);

  butter_sampler_desc_t target_sampler_desc =
      butter_sampler_desc_linear_clamp();
  context->default_sampler =
      butter_create_sampler(context, &target_sampler_desc);

  if (context->default_sampler == VK_NULL_HANDLE) {
    butter_log_fatal("Failed to create default sampler");
    return false;
  }

  butter_sampler_desc_t default_sampler_desc =
      butter_sampler_desc_nearest_clamp();
  vk_sampler_t default_sampler =
      butter_create_sampler(context, &default_sampler_desc);

  if (default_sampler == VK_NULL_HANDLE) {
    butter_log_fatal("Failed to create default sampler for fallback texture");
    return false;
  }

#define CHECKER_SIZE 2
#define CHECKER_PIXELS (CHECKER_SIZE * CHECKER_SIZE)
  u8 checker_data[CHECKER_PIXELS * 4] = {
      255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255,
  };

  struct butter_texture *default_texture = butter_create_texture(
      context, CHECKER_SIZE, CHECKER_SIZE, VK_FORMAT_R8G8B8A8_SRGB,
      checker_data, sizeof(checker_data), default_sampler);

  if (default_texture->image == VK_NULL_HANDLE) {
    butter_log_fatal("Failed to create default fallback texture");
    return false;
  }

  context->texture_registry.entries[0].id = 0;
  context->texture_registry.entries[0].texture = default_texture;
  context->texture_registry.count = 1;

  return true;
}

//
//
//

/**
 * @brief Destroy the texture registry.
 * @details Destroys the texture registry, destroys the default texture,
 * texture descriptor pool, and texture descriptor set layout if they exist,
 * if they do not, it's a simple no-op.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 */
static void butter_destroy_textures(butter_context_t *context) {
  if (context->texture_registry.count > 0) {
    butter_destroy_sampler(
        context, context->texture_registry.entries[0].texture->sampler);
    butter_destroy_texture(context,
                           context->texture_registry.entries[0].texture);
  }

  if (context->default_sampler)
    butter_destroy_sampler(context, context->default_sampler);

  for (u32 i = 0; i < context->texture_descriptor_pool_count; i++)
    if (context->texture_descriptor_pools[i])
      vkDestroyDescriptorPool(context->device,
                              context->texture_descriptor_pools[i], null);

  if (context->texture_descriptor_set_layout)
    vkDestroyDescriptorSetLayout(context->device,
                                 context->texture_descriptor_set_layout, null);

  mtx_destroy(&context->texture_descriptor_mutex);
}

//
//
//

vk_instance_t butter_create_instance(arena_t *arena, const cstr *app_name,
                                     b32 validation, butter_backend_t backend) {
  if (butter_is_vulkan_available() == false) {
    butter_log_fatal("Vulkan not available");
    return VK_NULL_HANDLE;
  }

  vk_result_t res;

  u32 header_version = VK_API_VERSION_1_0;
  u32 api_version = VK_API_VERSION_1_0;
  u32 driver_version = 0;
  if ((res = vkEnumerateInstanceVersion(&driver_version)) != VK_SUCCESS)
    butter_log_error("Could not get driver version, using 1.0: %d", res);
  else {
#ifdef VK_API_VERSION_1_4
    header_version = VK_API_VERSION_1_4;
#elif defined(VK_API_VERSION_1_3)
    header_version = VK_API_VERSION_1_3;
#elif defined(VK_API_VERSION_1_2)
    header_version = VK_API_VERSION_1_2;
#elif defined(VK_API_VERSION_1_1)
    header_version = VK_API_VERSION_1_1;
#else
    header_version = VK_API_VERSION_1_0;
#endif

    api_version =
        (driver_version > header_version) ? header_version : driver_version;
  }

  butter_log_debug(
      "Vulkan driver version: %d.%d.%d, using API: %d.%d.%d",
      VK_API_VERSION_MAJOR(driver_version),
      VK_API_VERSION_MINOR(driver_version),
      VK_API_VERSION_PATCH(driver_version), VK_API_VERSION_MAJOR(api_version),
      VK_API_VERSION_MINOR(api_version), VK_API_VERSION_PATCH(api_version));

  u32 extension_count = 0;
  const char *const *exts =
      butter_get_required_instance_extensions(backend, &extension_count);
  if (exts == null || extension_count == 0) {
    butter_log_fatal("Could not get required instance extensions");
    return VK_NULL_HANDLE;
  }

  u32 total_ext_count = extension_count + (validation ? 1 : 0);
  const cstr **all_exts =
      arena_alloc_zeroed(arena, const cstr *, total_ext_count);
  if (!all_exts) {
    butter_log_fatal("Could not allocate all_exts");
    return VK_NULL_HANDLE;
  }

  memcpy(all_exts, exts, sizeof(const cstr *) * extension_count);
  if (validation)
    all_exts[extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

  const cstr *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
  u32 layer_count = validation ? 1 : 0;

  vk_application_info_t app_info = {0};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = app_name;
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "butter";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = api_version;

  vk_instance_create_info_t instance_create_info = {0};
  instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo = &app_info;
  instance_create_info.enabledLayerCount = layer_count;
  instance_create_info.ppEnabledLayerNames = validation_layers;
  instance_create_info.enabledExtensionCount = total_ext_count;
  instance_create_info.ppEnabledExtensionNames = all_exts;

  vk_instance_t instance = VK_NULL_HANDLE;

  if ((res = vkCreateInstance(&instance_create_info, null, &instance)) !=
      VK_SUCCESS) {
    butter_log_fatal("Could not create instance: %d\n", res);
    return VK_NULL_HANDLE;
  }

  return instance;
}

butter_context_t *butter_create(arena_t *arena, vk_instance_t instance,
                                const butter_surface_info_t *surface_info,
                                const struct butter_init_config *config) {
  vk_result_t res;

  u64 dynamic_vbo_size = config->dynamic_vbo_size;
  u64 dynamic_ibo_size = config->dynamic_ibo_size;
  const cstr *pipeline_cache_path = config->pipeline_cache_path;
  b32 enable_depth = config->enable_depth;
  u32 latency_cap = config->latency_cap;
  u32 width = config->width;
  u32 height = config->height;

  if (latency_cap == 0)
    latency_cap = BUTTER_LATENCY_CAP;

  if (dynamic_vbo_size < BUTTER_DYNAMIC_VBO_MIN)
    dynamic_vbo_size = BUTTER_DYNAMIC_VBO_MIN;

  if (dynamic_ibo_size < BUTTER_DYNAMIC_IBO_MIN)
    dynamic_ibo_size = BUTTER_DYNAMIC_IBO_MIN;

  butter_context_t *context = arena_alloc_zeroed(arena, butter_context_t, 1);
  if (!context) {
    butter_log_fatal("Could not allocate butter context");
    return null;
  }

  u32 driver_version = 0;
  if ((res = vkEnumerateInstanceVersion(&driver_version)) != VK_SUCCESS)
    butter_log_error("Could not get driver version, using 1.0: %d", res);

  context->driver_version = driver_version;
  context->instance = instance;
  context->arena = arena;

  context->swapchain_arena = arena_new(MiB(1), KiB(64));
  if (!context->swapchain_arena) {
    butter_log_fatal("Could not create swapchain arena");
    goto fail;
  }

  context->attachment_arena = arena_new(MiB(1), KiB(64));
  if (!context->attachment_arena) {
    butter_log_fatal("Could not create attachment arena");
    goto fail;
  }

  context->enable_depth = enable_depth;
  context->aa_samples = 1;

  context->max_render_width = config->max_render_width;
  context->max_render_height = config->max_render_height;
  context->aa_vram_budget =
      config->aa_vram_budget ? config->aa_vram_budget : BUTTER_AA_VRAM_BUDGET;

  if (mtx_init(&context->aa_mutex, mtx_plain) != thrd_success)
    goto fail;

  if (pipeline_cache_path) {
    u64 len = strlen(pipeline_cache_path) + 1;
    context->pipeline_cache_path =
        (const cstr *)arena_alloc(context->arena, char, len);
    memcpy((void *)context->pipeline_cache_path, pipeline_cache_path, len);
  }

  context->surface = create_platform_surface(context->instance, surface_info);
  if (!context->surface) {
    butter_log_fatal("Could not create platform surface");
    goto fail;
  }

  if (!butter_select_physical_device(arena, context))
    goto fail;

  context->aa_mode = config->aa_mode;
  butter_set_aa_samples(context, config->aa_samples);

  if (!butter_create_device(context))
    goto fail;

  u32 mode_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      context->physical_device, context->surface, &mode_count, NULL);

  if (mode_count == 0) {
    butter_log_fatal("Could not get surface present modes");
    goto fail;
  }

  butter_log_debug("Available present modes count: %d", mode_count);
  vk_present_mode_khr_t *available_modes =
      arena_alloc_zeroed(context->arena, vk_present_mode_khr_t, mode_count);
  if ((res = vkGetPhysicalDeviceSurfacePresentModesKHR(
           context->physical_device, context->surface, &mode_count,
           available_modes)) != VK_SUCCESS) {
    butter_log_error("Could not get surface present modes: %d", res);
  }

  context->available_mode_count = mode_count;
  context->available_modes = available_modes;

  context->dynamic_ibo_size = dynamic_ibo_size;
  context->dynamic_vbo_size = dynamic_vbo_size;

  vk_command_pool_create_info_t cmd_pool_info = {0};
  cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmd_pool_info.queueFamilyIndex = context->queue_family;
  cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if ((res = vkCreateCommandPool(context->device, &cmd_pool_info, null,
                                 &context->cmd_pool)) != VK_SUCCESS) {
    butter_log_fatal("Could not create command pool: %d", res);
    goto fail;
  }

  if (!butter_create_swapchain(context, latency_cap, width, height))
    goto fail;

  vk_pipeline_cache_create_info_t pipeline_cache_info = {0};
  pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

  if (context->pipeline_cache_path) {
    string *cache_data =
        butter_read_cache_file(context->arena, context->pipeline_cache_path);
    if (cache_data) {
      pipeline_cache_info.pInitialData = cache_data->base;
      pipeline_cache_info.initialDataSize = cache_data->len;
    }
  }

  if ((res = vkCreatePipelineCache(context->device, &pipeline_cache_info, NULL,
                                   &context->pipeline_cache)) != VK_SUCCESS) {
    butter_log_warning(
        "Failed to create pipeline cache – pipelines will not be cached: %d",
        res);
    context->pipeline_cache = VK_NULL_HANDLE;
  }

  vk_command_pool_create_info_t pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = context->queue_family;
  pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if ((res = vkCreateCommandPool(context->device, &pool_info, null,
                                 &context->upload_pool_sync)) != VK_SUCCESS) {
    butter_log_error("Could not create synchronous upload pool: %d", res);
    context->upload_pool_sync = VK_NULL_HANDLE;
  }

  if (mtx_init(&context->render_mutex, mtx_plain) != thrd_success)
    goto fail;

  if (cnd_init(&context->frame_ready) != thrd_success)
    goto fail;

  if (cnd_init(&context->frame_done) != thrd_success)
    goto fail;

  if (!butter_init_textures(context, arena))
    goto fail;

  context->in_flight_frame_slot = 0;
  return context;

fail:
  butter_log_fatal("Could not create butter context");
  butter_destroy(context);
  return null;
}

void butter_destroy(butter_context_t *context) {
  if (!context) {
    butter_log_debug("Butter context already destroyed");
    return;
  }

  butter_stop_texture_uploads(context);
  butter_stop_render_thread(context);

  vk_result_t res;

  if (context->device != VK_NULL_HANDLE) {
    res = vkDeviceWaitIdle(context->device);
    if (res != VK_SUCCESS)
      butter_log_error("Could not wait for device idle");
  }

  butter_destroy_targets(context);

  if (context->dynamic_vbos)
    for (u32 i = 0; i < context->dynamic_cap; i++)
      butter_destroy_buffer(context, &context->dynamic_vbos[i]);

  if (context->dynamic_ibos)
    for (u32 i = 0; i < context->dynamic_cap; i++)
      butter_destroy_buffer(context, &context->dynamic_ibos[i]);

  butter_destroy_swapchain_resources(context);

  if (context->cmd_pool) {
    vkDestroyCommandPool(context->device, context->cmd_pool, null);
    context->cmd_pool = VK_NULL_HANDLE;
  }

  if (context->render_pass) {
    butter_log_debug("Destroying render pass");
    vkDestroyRenderPass(context->device, context->render_pass, null);
  }

  if (context->render_pass_load) {
    butter_log_debug("Destroying render pass load");
    vkDestroyRenderPass(context->device, context->render_pass_load, null);
  }

  if (context->render_pass_target) {
    butter_log_debug("Destroying render pass target");
    vkDestroyRenderPass(context->device, context->render_pass_target, null);
  }

  mtx_destroy(&context->render_mutex);
  cnd_destroy(&context->frame_ready);
  cnd_destroy(&context->frame_done);

  butter_destroy_textures(context);

  if (context->pipeline_cache && context->pipeline_cache_path) {
    u64 data_size = 0;
    if (vkGetPipelineCacheData(context->device, context->pipeline_cache,
                               &data_size, null) == VK_SUCCESS &&
        data_size > 0) {
      u8 *data = arena_alloc(context->arena, u8, data_size);
      if (vkGetPipelineCacheData(context->device, context->pipeline_cache,
                                 &data_size, data) == VK_SUCCESS)
        if (!butter_write_cache_file(context->pipeline_cache_path, data,
                                     data_size))
          butter_log_warning("Could not persist pipeline cache");
    }
  }

  if (context->pipeline_cache)
    vkDestroyPipelineCache(context->device, context->pipeline_cache, null);
  context->pipeline_cache = VK_NULL_HANDLE;

  if (context->upload_pool_sync)
    vkDestroyCommandPool(context->device, context->upload_pool_sync, null);
  context->upload_pool_sync = VK_NULL_HANDLE;

  if (context->upload_pool_async)
    vkDestroyCommandPool(context->device, context->upload_pool_async, null);
  context->upload_pool_async = VK_NULL_HANDLE;

  if (context->swapchain)
    vkDestroySwapchainKHR(context->device, context->swapchain, null);
  context->swapchain = VK_NULL_HANDLE;

  while (context->pipeline_count > 0)
    butter_destroy_pipeline(context,
                            context->pipelines[context->pipeline_count - 1]);

  butter_stats_destroy(context);

  butter_log_debug("Destroying device %p (instance %p)",
                   (void *)context->device, (void *)context->instance);
  if (context->device)
    vkDestroyDevice(context->device, null);
  context->device = VK_NULL_HANDLE;

  if (context->surface)
    vkDestroySurfaceKHR(context->instance, context->surface, null);
  context->surface = VK_NULL_HANDLE;

  if (context->instance)
    vkDestroyInstance(context->instance, null);
  context->instance = VK_NULL_HANDLE;

  if (context->attachment_arena)
    arena_free(context->attachment_arena);
  if (context->swapchain_arena)
    arena_free(context->swapchain_arena);
}

b32 butter_ensure_dynamic_buffers(butter_context_t *context) {
  if (context->frames_in_flight <= context->dynamic_cap)
    return true;

  u32 old_cap = context->dynamic_cap;
  u32 cap = context->frames_in_flight;

  struct butter_buffer *vbos =
      arena_alloc_zeroed(context->arena, struct butter_buffer, cap);
  struct butter_buffer *ibos =
      arena_alloc_zeroed(context->arena, struct butter_buffer, cap);

  for (u32 i = 0; i < old_cap; i++) {
    vbos[i] = context->dynamic_vbos[i];
    ibos[i] = context->dynamic_ibos[i];
  }

  context->dynamic_vbos = vbos;
  context->dynamic_ibos = ibos;
  context->dynamic_cap = cap;

  for (u32 i = old_cap; i < cap; i++) {
    vbos[i] = butter_create_buffer(context, context->dynamic_vbo_size,
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
    if (vbos[i].handle == VK_NULL_HANDLE) {
      butter_log_fatal("Could not create dynamic VBO");
      return false;
    }

    ibos[i] = butter_create_buffer(context, context->dynamic_ibo_size,
                                   VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true);
    if (ibos[i].handle == VK_NULL_HANDLE) {
      butter_log_fatal("Could not create dynamic IBO");
      return false;
    }
  }

  return true;
}

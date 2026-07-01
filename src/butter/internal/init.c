#include <string.h>

#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/graphics.h>
#include <butter/texture.h>

#include <butter/internal/check.h>
#include <butter/internal/device.h>
#include <butter/internal/get.h>
#include <butter/internal/init.h>
#include <butter/internal/swapchain.h>
#include <butter/internal/types.h>

#include <butter/log.h>

#ifdef BUTTER_X11
#include <vulkan/vulkan_xcb.h>
#include <xcb/xcb.h>
#endif

#ifdef BUTTER_WAYLAND
#include <vulkan/vulkan_wayland.h>
#include <wayland-client.h>
#endif

#include <vulkan/vulkan_core.h>

#ifndef BUTTER_MAX_TEXTURES
#define BUTTER_MAX_TEXTURES (1024)
#endif

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
                                u32 latency_cap, u32 width, u32 height) {
  vk_result_t res;

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

  context->surface = create_platform_surface(context->instance, surface_info);
  if (!context->surface) {
    butter_log_fatal("Could not create platform surface");
    return null;
  }

  if (!butter_select_physical_device(arena, context))
    goto fail;
  if (!butter_create_device(context))
    goto fail;

  u32 mode_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      context->physical_device, context->surface, &mode_count, NULL);

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

  if (!butter_create_swapchain(context, latency_cap, width, height))
    goto fail;

  vk_command_pool_create_info_t pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = context->queue_family;
  pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

  vk_pipeline_cache_create_info_t pipeline_cache_info = {0};
  pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

  if ((res = vkCreatePipelineCache(context->device, &pipeline_cache_info, NULL,
                                   &context->pipeline_cache)) != VK_SUCCESS) {
    butter_log_warning(
        "Failed to create pipeline cache – pipelines will not be cached: %d",
        res);
    context->pipeline_cache = VK_NULL_HANDLE;
  }

  if ((res = vkCreateCommandPool(context->device, &pool_info, null,
                                 &context->upload_pool_sync)) != VK_SUCCESS) {
    butter_log_error("Could not create synchronous upload pool: %d", res);
    context->upload_pool_sync = VK_NULL_HANDLE;
  }

  context->dynamic_vbo_size = MiB(8);
  context->dynamic_vbo_offset = 0;
  context->dynamic_vbos = arena_alloc_zeroed(
      context->arena, struct butter_buffer, context->image_count);

  for (u32 i = 0; i < context->image_count; i++) {
    context->dynamic_vbos[i] =
        butter_create_buffer(context, context->dynamic_vbo_size,
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
    if (context->dynamic_vbos[i].handle == VK_NULL_HANDLE) {
      butter_log_fatal("Could not create dynamic VBO");
      return null;
    }
  }

  if ((context->available_vulkan_features &
       BUTTER_FEATURE_TIMELINE_SEMAPHORE) == 0) {
    vk_semaphore_create_info_t semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    vk_fence_create_info_t fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    context->rendering_finished = arena_alloc_zeroed(
        context->arena, vk_semaphore_t, context->image_count);
    context->image_available = arena_alloc_zeroed(
        context->arena, vk_semaphore_t, context->image_count);
    context->in_flight_fences =
        arena_alloc_zeroed(context->arena, vk_fence_t, context->image_count);

    for (u32 i = 0; i < context->image_count; i++) {
      if ((res = vkCreateSemaphore(context->device, &semaphore_info, null,
                                   &context->rendering_finished[i])) !=
          VK_SUCCESS)
        butter_log_error("Could not create rendering finished semaphore: %d",
                         res);
      if ((res = vkCreateSemaphore(context->device, &semaphore_info, null,
                                   &context->image_available[i])) != VK_SUCCESS)
        butter_log_error("Could not create image available semaphore: %d", res);
      if ((res = vkCreateFence(context->device, &fence_info, null,
                               &context->in_flight_fences[i])) != VK_SUCCESS)
        butter_log_error("Could not create in flight fence: %d", res);
    }
  } else {
#ifdef VK_API_VERSION_1_2
    vk_semaphore_create_info_t old_semaphore_info = {0};
    old_semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    context->rendering_finished = arena_alloc_zeroed(
        context->arena, vk_semaphore_t, context->image_count);
    context->image_available = arena_alloc_zeroed(
        context->arena, vk_semaphore_t, context->image_count);

    for (u32 i = 0; i < context->image_count; i++) {
      if ((res = vkCreateSemaphore(context->device, &old_semaphore_info, null,
                                   &context->rendering_finished[i])) !=
          VK_SUCCESS)
        butter_log_error("Could not create rendering finished semaphore: %d",
                         res);
      if ((res = vkCreateSemaphore(context->device, &old_semaphore_info, null,
                                   &context->image_available[i])) != VK_SUCCESS)
        butter_log_error("Could not create image available semaphore: %d", res);
    }

    vk_semaphore_type_create_info_t semaphore_type_info = {0};
    semaphore_type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    semaphore_type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    semaphore_type_info.initialValue = 0;

    vk_semaphore_create_info_t semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_info.pNext = &semaphore_type_info;

    vkCreateSemaphore(context->device, &semaphore_info, null,
                      &context->timeline_semaphore);
    context->timeline_value = 0;
    butter_log_debug("Timeline semaphore initialized to 0");
#else
    butter_log_fatal("How did you get here?");
    return null;
#endif
  }

  mtx_init(&context->render_mutex, mtx_plain);
  cnd_init(&context->frame_ready);
  cnd_init(&context->frame_done);

  context->texture_registry.capacity = BUTTER_MAX_TEXTURES;
  context->texture_registry.entries =
      arena_alloc_zeroed(context->arena, struct butter_texture_registry_entry,
                         BUTTER_MAX_TEXTURES);
  context->texture_registry.count = 0;
  context->texture_registry.next_id = 1;

  vk_descriptor_pool_size_t pool_size = {0};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = BUTTER_MAX_TEXTURES;

  context->texture_descriptor_pool = butter_create_descriptor_pool(
      context, BUTTER_MAX_TEXTURES, &pool_size, 1);

  vk_descriptor_set_layout_binding_t binding = {0};
  binding.binding = 0;
  binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  binding.descriptorCount = 1;
  binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  context->texture_descriptor_set_layout =
      butter_create_descriptor_set_layout(context, &binding, 1);

  butter_sampler_desc_t default_sampler_desc =
      butter_sampler_desc_nearest_clamp();
  VkSampler default_sampler =
      butter_create_sampler(context, &default_sampler_desc);
  if (default_sampler == VK_NULL_HANDLE) {
    butter_log_fatal("Failed to create default sampler for fallback texture");
    return NULL;
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
    return NULL;
  }

  butter_descriptor_set_t default_desc =
      butter_allocate_descriptor_set(context, context->texture_descriptor_pool,
                                     context->texture_descriptor_set_layout);

  butter_update_descriptor_image(context, &default_desc, 0,
                                 default_texture->view,
                                 default_texture->sampler);
  default_texture->descriptor_set = default_desc;

  context->texture_registry.entries[0].id = 0;
  context->texture_registry.entries[0].texture = default_texture;
  context->texture_registry.count = 1;

  context->frame_index = 0;
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

  vk_result_t res;

  if (context->device != VK_NULL_HANDLE) {
    res = vkDeviceWaitIdle(context->device);
    if (res != VK_SUCCESS)
      butter_log_error("Could not wait for device idle");
  }

  if (context->dynamic_vbos)
    for (u32 i = 0; i < context->image_count; i++)
      butter_destroy_buffer(context, &context->dynamic_vbos[i]);

  butter_destroy_swapchain_resources(context);

  if (context->render_pass) {
    butter_log_debug("Destroying render pass");
    vkDestroyRenderPass(context->device, context->render_pass, null);
  }

  if (context->rendering_finished && context->image_count > 0) {
    for (u32 i = 0; i < context->image_count; i++) {
      if (context->rendering_finished[i])
        vkDestroySemaphore(context->device, context->rendering_finished[i],
                           null);
      if (context->image_available[i])
        vkDestroySemaphore(context->device, context->image_available[i], null);
      if (context->in_flight_fences[i])
        vkDestroyFence(context->device, context->in_flight_fences[i], null);
    }
  }

  context->rendering_finished = null;
  context->image_available = null;
  context->in_flight_fences = null;

  mtx_destroy(&context->render_mutex);
  cnd_destroy(&context->frame_ready);
  cnd_destroy(&context->frame_done);

  if (context->texture_registry.count > 0) {
    butter_destroy_texture(context,
                           context->texture_registry.entries[0].texture);
  }

  if (context->texture_descriptor_pool)
    vkDestroyDescriptorPool(context->device, context->texture_descriptor_pool,
                            null);

  if (context->texture_descriptor_set_layout)
    vkDestroyDescriptorSetLayout(context->device,
                                 context->texture_descriptor_set_layout, null);

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

  if (context->device)
    vkDestroyDevice(context->device, null);
  context->device = VK_NULL_HANDLE;

  if (context->surface)
    vkDestroySurfaceKHR(context->instance, context->surface, null);
  context->surface = VK_NULL_HANDLE;

  if (context->instance)
    vkDestroyInstance(context->instance, null);
  context->instance = VK_NULL_HANDLE;
}

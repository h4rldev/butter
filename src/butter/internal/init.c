#include <string.h>

#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/internal/check.h>
#include <butter/internal/device.h>
#include <butter/internal/get.h>
#include <butter/internal/init.h>
#include <butter/internal/swapchain.h>
#include <butter/internal/types.h>

#include <butter/log.h>

#include <wayland-client.h>
#include <xcb/xcb.h>

#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_xcb.h>

static vk_surface_khr_t
create_platform_surface(vk_instance_t instance,
                        const butter_surface_info_t *info) {
  vk_surface_khr_t surface = VK_NULL_HANDLE;
  switch (info->backend) {
  case BUTTER_BACKEND_XCB: {
    vk_xcb_surface_create_info_khr_t xcb_info = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = (xcb_connection_t *)info->display,
        .window = (xcb_window_t)(uintptr_t)info->handle,
    };

    vk_result_t res =
        vkCreateXcbSurfaceKHR(instance, &xcb_info, null, &surface);
    if (res != VK_SUCCESS)
      butter_log_error("Could not create xcb surface");
  } break;
  case BUTTER_BACKEND_WAYLAND: {
    vk_wayland_surface_create_info_khr_t wayland_info = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = (struct wl_display *)info->display,
        .surface = (struct wl_surface *)info->handle,
    };
    vk_result_t res =
        vkCreateWaylandSurfaceKHR(instance, &wayland_info, null, &surface);
    if (res != VK_SUCCESS)
      butter_log_error("Could not create wayland surface");
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

  vk_application_info_t app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = app_name,
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "butter",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  vk_instance_create_info_t instance_create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledLayerCount = layer_count,
      .ppEnabledLayerNames = validation_layers,
      .enabledExtensionCount = total_ext_count,
      .ppEnabledExtensionNames = all_exts,
  };

  vk_instance_t instance = VK_NULL_HANDLE;
  vk_result_t res = vkCreateInstance(&instance_create_info, null, &instance);
  if (res != VK_SUCCESS) {
    butter_log_fatal("Could not create instance: %d\n", res);
    return VK_NULL_HANDLE;
  }

  return instance;
}

butter_context_t *butter_create(arena_t *arena, vk_instance_t instance,
                                const butter_surface_info_t *surface_info,
                                u32 latency_cap, u32 width, u32 height) {
  butter_context_t *context = arena_alloc_zeroed(arena, butter_context_t, 1);
  if (!context) {
    butter_log_fatal("Could not allocate butter context");
    return null;
  }

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
  if (!butter_create_swapchain(arena, context, latency_cap, width, height))
    goto fail;

  vk_semaphore_create_info_t semaphore_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  vk_fence_create_info_t fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  context->rendering_finished =
      arena_alloc_zeroed(arena, vk_semaphore_t, context->image_count);
  context->image_available =
      arena_alloc_zeroed(arena, vk_semaphore_t, context->image_count);
  context->in_flight_fences =
      arena_alloc_zeroed(arena, vk_fence_t, context->image_count);

  for (u32 i = 0; i < context->image_count; i++) {
    vk_result_t res = vkCreateSemaphore(context->device, &semaphore_info, null,
                                        &context->rendering_finished[i]);
    if (res != VK_SUCCESS)
      butter_log_error("Could not create rendering finished semaphore: %d",
                       res);

    res = vkCreateSemaphore(context->device, &semaphore_info, null,
                            &context->image_available[i]);
    if (res != VK_SUCCESS)
      butter_log_error("Could not create image available semaphore: %d", res);
    res = vkCreateFence(context->device, &fence_info, null,
                        &context->in_flight_fences[i]);
    if (res != VK_SUCCESS)
      butter_log_error("Could not create in flight fence: %d", res);
  }

  mtx_init(&context->render_mutex, mtx_plain);
  cnd_init(&context->frame_ready);
  cnd_init(&context->frame_done);

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

  butter_destroy_swapchain_resources(context);

  if (context->render_pass)
    vkDestroyRenderPass(context->device, context->render_pass, null);
  context->render_pass = VK_NULL_HANDLE;

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

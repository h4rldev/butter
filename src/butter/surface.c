#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include "butter_internal.h"
#include <butter/device.h>
#include <butter/surface.h>
#include <butter/swapchain.h>
#include <butter/types.h>

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

    vkCreateXcbSurfaceKHR(instance, &xcb_info, null, &surface);
  } break;
  case BUTTER_BACKEND_WAYLAND: {
    vk_wayland_surface_create_info_khr_t wayland_info = {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = (struct wl_display *)info->display,
        .surface = (struct wl_surface *)info->handle,
    };
    vkCreateWaylandSurfaceKHR(instance, &wayland_info, null, &surface);
  } break;
  }

  return surface;
}

butter_context_t *butter_create(arena_t *arena, vk_instance_t instance,
                                const butter_surface_info_t *surface_info) {
  butter_context_t *context = arena_alloc(arena, butter_context_t, 1);
  if (!context)
    return null;

  context->backend = surface_info->backend;
  context->instance = instance;

  context->surface = create_platform_surface(context->instance, surface_info);
  if (!context->surface)
    return null;

  if (!butter_select_physical_device(arena, context))
    goto fail;
  if (!butter_create_device(context))
    goto fail;
  if (!butter_create_swapchain(arena, context))
    goto fail;

  vk_semaphore_create_info_t semaphore_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  vk_fence_create_info_t fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  for (u32 i = 0; i < BUTTER_MAX_FRAMES; i++) {
    vkCreateSemaphore(context->device, &semaphore_info, null,
                      &context->rendering_finished[i]);
    vkCreateSemaphore(context->device, &semaphore_info, null,
                      &context->image_available[i]);
    vkCreateFence(context->device, &fence_info, null,
                  &context->in_flight_fences[i]);
  }

  context->frame_index = 0;

  return context;

fail:
  butter_destroy(context);
  return null;
}

void butter_destroy(butter_context_t *context) {
  if (!context)
    return;

  vkDeviceWaitIdle(context->device);

  for (u32 i = 0; i < BUTTER_MAX_FRAMES; i++) {
    if (context->rendering_finished[i])
      vkDestroySemaphore(context->device, context->rendering_finished[i], null);
    if (context->image_available[i])
      vkDestroySemaphore(context->device, context->image_available[i], null);
    if (context->in_flight_fences[i])
      vkDestroyFence(context->device, context->in_flight_fences[i], null);
  }

  if (context->framebuffers)
    for (u32 i = 0; i < context->image_count; i++)
      if (context->framebuffers[i])
        vkDestroyFramebuffer(context->device, context->framebuffers[i], null);

  if (context->image_views)
    for (u32 i = 0; i < context->image_count; i++)
      if (context->image_views[i])
        vkDestroyImageView(context->device, context->image_views[i], null);

  if (context->render_pass)
    vkDestroyRenderPass(context->device, context->render_pass, null);
  if (context->swapchain)
    vkDestroySwapchainKHR(context->device, context->swapchain, null);
  if (context->device)
    vkDestroyDevice(context->device, null);
  if (context->surface)
    vkDestroySurfaceKHR(context->instance, context->surface, null);
  if (context->instance)
    vkDestroyInstance(context->instance, null);
}

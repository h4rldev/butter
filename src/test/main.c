#include "butter/present.h"
#include <stdio.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/string.h>

#include <bread/backend.h>
#include <bread/event.h>
#include <bread/window.h>

#include <butter/check.h>
#include <butter/device.h>
#include <butter/instance.h>
#include <butter/surface.h>
#include <butter/swapchain.h>
#include <butter/types.h>

#define enable_validation true

static b32 resize_occurred = false;

void bread_event_callback(bread_event_t *event, void *userdata) {
  switch (event->type) {
  case BREAD_EVENT_WINDOW_RESIZE:
    resize_occurred = true;
    break;
  default:
    // fprintf(stderr, "Got event of type %d\n", event->type);
    break;
  }
}
int main(void) {
  arena_t *arena = arena_new(GiB(4), MiB(256));

  bread_window_t window = {
      .width = 800,
      .height = 600,
      .title = HTILS_STR("butter test"),
      .arena = arena,
  };

  bread_window_set_event_callback(&window, bread_event_callback, null);
  bread_window_init(&window);

  if (butter_is_vulkan_available() == false) {
    fprintf(stderr, "Vulkan not available\n");
    return 1;
  }

  bread_backend_type_t be = bread_get_backend_type();
  butter_backend_t backend =
      (be == BREAD_BACKEND_X11) ? BUTTER_BACKEND_XCB : BUTTER_BACKEND_WAYLAND;

  butter_surface_info_t surface_info = {
      .backend = backend,
      .handle = bread_window_get_surface(&window).handle,
      .display = bread_window_get_surface(&window).display,
  };

  vk_instance_t instance =
      butter_create_instance(arena, "butter", true, backend);
  butter_context_t *context = butter_create(arena, instance, &surface_info);
  if (!context) {
    fprintf(stderr, "Could not create context\n");
    vkDestroyInstance(instance, null);
    return 1;
  }

  vk_device_t device = butter_get_device(context);
  u32 queue_family = butter_get_queue_family(context);

  VkCommandPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queue_family,
  };

  VkCommandPool command_pool;
  vkCreateCommandPool(device, &pool_info, null, &command_pool);

  VkCommandBufferAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 3,
  };

  VkCommandBuffer cmds[3];
  vkAllocateCommandBuffers(device, &alloc_info, cmds);

  VkClearValue clear = {{{0.2f, 0.3f, 0.8f, 1.0f}}}; // blue-ish

  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  VkRenderPassBeginInfo rp_begin = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = butter_get_default_render_pass(context),
      .renderArea = {{0, 0}, butter_get_extent(context)},
      .clearValueCount = 1,
      .pClearValues = &clear,
  };

  while (bread_window_should_close(&window) == false) {
    bread_window_poll(&window);

    if (resize_occurred) {
      butter_update_surface(arena, context);

      rp_begin.renderPass = butter_get_default_render_pass(context);
      rp_begin.renderArea.extent = butter_get_extent(context);
      resize_occurred = false;
    }

    u32 image_index = 0;
    vk_result_t res = butter_acquire_next_image(context, &image_index);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
      butter_update_surface(arena, context);
      rp_begin.renderPass = butter_get_default_render_pass(context);
      rp_begin.renderArea.extent = butter_get_extent(context);
      continue;
    } else if (res != VK_SUCCESS || res == VK_SUBOPTIMAL_KHR) {
      break;
    }

    u32 frame_index = butter_get_frame_index(context);
    VkCommandBuffer cmd = cmds[frame_index];

    vkResetCommandBuffer(cmd, 0);
    vkBeginCommandBuffer(cmd, &begin_info);

    rp_begin.framebuffer = butter_get_framebuffer(context, image_index);
    vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    res = butter_submit_and_present(context, cmd, image_index);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
      butter_update_surface(arena, context);
      rp_begin.renderPass = butter_get_default_render_pass(context);
      rp_begin.renderArea.extent = butter_get_extent(context);
    }
  }

  vkDeviceWaitIdle(device);
  vkDestroyCommandPool(device, command_pool, null);
  butter_destroy(context);
  bread_window_destroy(&window);
  arena_free(arena);

  return 0;
}

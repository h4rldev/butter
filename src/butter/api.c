#ifdef BUTTER_X11
#include <stdlib.h>
#endif

#include <butter/internal/check.h>
#include <butter/internal/init.h>
#include <butter/internal/present.h>
#include <butter/internal/swapchain.h>
#include <butter/internal/types.h>

#include <butter/api.h>
#include <butter/types.h>

#ifndef BUTTER_LATENCY_CAP
#define BUTTER_LATENCY_CAP 4
#endif

butter_t *butter_init(arena_t *arena, butter_surface_info_t *surface_info,
                      cstr *app_name, b32 use_validation_layers, u32 width,
                      u32 height) {
  if (!arena || !surface_info)
    return null;

  if (!butter_is_vulkan_available())
    return null;

  vk_instance_t instance = butter_create_instance(
      arena, app_name, use_validation_layers, surface_info->backend);
  if (!instance)
    return null;

  butter_t *butter = butter_create(arena, instance, surface_info,
                                   BUTTER_LATENCY_CAP, width, height);
  if (!butter)
    return null;

  vk_command_pool_create_info_t pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = butter->queue_family,
  };

  vk_result_t res =
      vkCreateCommandPool(butter->device, &pool_info, null, &butter->cmd_pool);
  if (res != VK_SUCCESS) {
    butter_destroy(butter);
    return null;
  }

  butter->cmds = arena_alloc(arena, vk_command_buffer_t, butter->image_count);
  vk_command_buffer_allocate_info_t alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = butter->cmd_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = butter->image_count,
  };

  res = vkAllocateCommandBuffers(butter->device, &alloc_info, butter->cmds);
  if (res != VK_SUCCESS) {
    vkDestroyCommandPool(butter->device, butter->cmd_pool, null);
    butter_destroy(butter);
    return null;
  }

#ifdef BUTTER_X11
  if (surface_info->backend == BUTTER_BACKEND_XCB) {
    xcb_connection_t *conn = (xcb_connection_t *)surface_info->display;
    xcb_generic_event_t *ev;
    while ((ev = xcb_poll_for_event(conn)) != null)
      free(ev);
  }
#endif

  return butter;
}

void butter_end(butter_t *butter) {
  if (!butter)
    return;
  vkDeviceWaitIdle(butter->device);
  vkDestroyCommandPool(butter->device, butter->cmd_pool, null);
  butter_destroy(butter);
}

void butter_set_clear_color(butter_t *butter, f32 r, f32 g, f32 b, f32 a) {
  butter->clear_color.color.float32[0] = r;
  butter->clear_color.color.float32[1] = g;
  butter->clear_color.color.float32[2] = b;
  butter->clear_color.color.float32[3] = a;
}

butter_frame_t *butter_begin_frame(arena_t *arena, butter_t *butter) {
  if (butter->resize_pending) {
    butter_resize(arena, butter, butter->pending_width, butter->pending_height);
    butter->resize_pending = false;
  }

  vk_extent2d_t extent = butter->extent;
  if (extent.width == 0 || extent.height == 0)
    return null;

  u32 image_index = 0;
  vk_result_t res = butter_acquire_next_image(butter, &image_index);
  if (res != VK_SUCCESS)
    return null;

  u32 frame_index = butter->frame_index;
  vk_command_buffer_t cmd = butter->cmds[frame_index];
  vk_command_buffer_begin_info_t begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkResetCommandBuffer(cmd, 0);
  vkBeginCommandBuffer(cmd, &begin_info);

  vk_render_pass_begin_info_t rp_begin = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = butter->render_pass,
      .framebuffer = butter->framebuffers[image_index],
      .renderArea.extent = extent,
      .renderArea.offset = {0, 0},
      .clearValueCount = 1,
      .pClearValues = &butter->clear_color,
  };
  vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

  static butter_frame_t frame;
  frame.cmd = cmd;
  frame.fb = butter->framebuffers[image_index];
  frame.image_index = image_index;
  frame.extent = extent;
  frame.rp = butter->render_pass;

  return &frame;
}

vk_result_t butter_end_frame(arena_t *arena, butter_t *butter,
                             butter_frame_t *frame) {
  vkCmdEndRenderPass(frame->cmd);
  vkEndCommandBuffer(frame->cmd);

  return butter_submit_and_present(butter, frame->cmd, frame->image_index);
}

void butter_resize(arena_t *arena, butter_t *butter, u32 width, u32 height) {
  vk_command_pool_create_info_t pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = butter->queue_family,
  };

  vkDeviceWaitIdle(butter->device);
  vkDestroyCommandPool(butter->device, butter->cmd_pool, null);
  butter_update_surface(arena, butter, BUTTER_LATENCY_CAP, width, height);

  vkCreateCommandPool(butter->device, &pool_info, null, &butter->cmd_pool);

  vk_command_buffer_allocate_info_t alloc_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = butter->cmd_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = butter->image_count,
  };

  butter->cmds = arena_alloc(arena, vk_command_buffer_t, butter->image_count);
  vkAllocateCommandBuffers(butter->device, &alloc_info, butter->cmds);
}

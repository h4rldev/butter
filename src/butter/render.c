#include <stdatomic.h>
#include <threads.h>

#include <butter/internal/check.h>
#include <butter/internal/init.h>
#include <butter/internal/present.h>
#include <butter/internal/swapchain.h>
#include <butter/internal/types.h>

#include <butter/log.h>
#include <butter/render.h>
#include <butter/types.h>

#ifndef BUTTER_LATENCY_CAP
#define BUTTER_LATENCY_CAP 4
#endif

static u64 get_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

static void butter_limit_frame_rate(butter_t *butter, u64 frame_start_ns) {
  if (!butter)
    return;

  f32 target = butter->target_refresh_rate;
  if (target <= 0.0f)
    return;

  u64 elapsed = get_time_ns() - frame_start_ns;
  u64 target_ns = (u64)(1e9f / target);

  if (elapsed < target_ns) {
    u64 sleep_ns = target_ns - elapsed;
    struct timespec ts = {
        .tv_sec = sleep_ns / 1000000000ULL,
        .tv_nsec = sleep_ns % 1000000000ULL,
    };

    thrd_sleep(&ts, NULL);
  }
}

butter_t *butter_init(arena_t *arena, butter_surface_info_t *surface_info,
                      cstr *app_name, b32 use_validation_layers, u32 width,
                      u32 height) {
  vk_result_t res;

  butter_log_debug("Initializing butter");
  if (!arena || !surface_info)
    return null;

  butter_log_debug("Checking for vulkan support");
  if (!butter_is_vulkan_available())
    return null;

  butter_log_debug("Creating vulkan instance");
  vk_instance_t instance = butter_create_instance(
      arena, app_name, use_validation_layers, surface_info->backend);
  if (!instance) {
    butter_log_fatal("Could not create vulkan instance");
    return null;
  }

  butter_log_debug("Creating butter context");
  butter_t *butter = butter_create(arena, instance, surface_info,
                                   BUTTER_LATENCY_CAP, width, height);
  if (!butter) {
    butter_log_fatal("Could not create butter context");
    return null;
  }

  butter_log_debug("Creating command pool");
  vk_command_pool_create_info_t pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = butter->queue_family;

  if ((res = vkCreateCommandPool(butter->device, &pool_info, null,
                                 &butter->cmd_pool)) != VK_SUCCESS) {
    butter_log_fatal("Could not create command pool");
    butter_destroy(butter);
    return null;
  }

  butter->cmds =
      arena_alloc_zeroed(arena, vk_command_buffer_t, butter->image_count);

  vk_command_buffer_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = butter->cmd_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = butter->image_count;

  butter_log_debug("Allocating command buffers");
  if ((res = vkAllocateCommandBuffers(butter->device, &alloc_info,
                                      butter->cmds)) != VK_SUCCESS) {
    butter_log_fatal("Could not allocate command buffers");
    vkDestroyCommandPool(butter->device, butter->cmd_pool, null);
    butter_destroy(butter);
    return null;
  }

  return butter;
}

void butter_end(butter_t *butter) {
  if (!butter) {
    butter_log_debug("Butter context already destroyed");
    return;
  }
  vkDeviceWaitIdle(butter->device);
  vkDestroyCommandPool(butter->device, butter->cmd_pool, null);
  butter_destroy(butter);
}

void butter_set_clear_color(butter_t *butter, f32 r, f32 g, f32 b, f32 a) {
  // butter_log_debug("Setting clear color to %f, %f, %f, %f\n", r, g, b, a);

  butter->clear_color.color.float32[0] = r;
  butter->clear_color.color.float32[1] = g;
  butter->clear_color.color.float32[2] = b;
  butter->clear_color.color.float32[3] = a;
}

butter_frame_t *butter_begin_frame(arena_t *arena, butter_t *butter) {
  vk_extent2d_t extent = butter->extent;
  if (extent.width == 0 || extent.height == 0) {
    butter_log_error("Extent is zero");
    return null;
  }

  u32 image_index = 0;
  vk_result_t res = butter_acquire_next_image(butter, &image_index);
  if (res != VK_SUCCESS) {
    if (res == VK_TIMEOUT)
      butter_log_debug("Acquire timed out - skipping frame");
    else if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
      butter_log_debug("Swapchain out of date - resizing");
    else
      butter_log_debug("Acquire error %d - triggering resize", res);

    butter->resize_pending = true;
    butter->pending_width = butter->extent.width;
    butter->pending_height = butter->extent.height;
    return NULL;
  }

  u32 frame_index = butter->frame_index;
  vk_command_buffer_t cmd = butter->cmds[frame_index];
  vk_command_buffer_begin_info_t begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  res = vkResetCommandBuffer(cmd, 0);
  if (res != VK_SUCCESS)
    butter_log_error("Could not reset command buffer");

  res = vkBeginCommandBuffer(cmd, &begin_info);
  if (res != VK_SUCCESS) {
    butter_log_error("Could not begin command buffer");
    return null;
  }

  vk_render_pass_begin_info_t rp_begin = {0};
  rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp_begin.renderPass = butter->render_pass;
  rp_begin.framebuffer = butter->framebuffers[image_index];
  rp_begin.renderArea.extent = extent;
  rp_begin.renderArea.offset = (vk_offset2d_t){0};
  rp_begin.clearValueCount = 1;
  rp_begin.pClearValues = &butter->clear_color;

  // butter_log_debug("Beginning render pass");
  vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

  // butter_log_debug("Creating frame");
  butter_frame_t *frame = arena_alloc_zeroed(arena, butter_frame_t, 1);
  frame->cmd = cmd;
  frame->fb = butter->framebuffers[image_index];
  frame->image_index = image_index;
  frame->extent = extent;
  frame->rp = butter->render_pass;
  frame->frame_start_ns = get_time_ns();

  return frame;
}

vk_result_t butter_end_frame(arena_t *arena, butter_t *butter,
                             butter_frame_t *frame) {
  // butter_log_debug("Ending frame");
  vkCmdEndRenderPass(frame->cmd);
  vkEndCommandBuffer(frame->cmd);

  butter_limit_frame_rate(butter, frame->frame_start_ns);

  // butter_log_debug("Submitting frame");
  return butter_submit_and_present(butter, frame->cmd, frame->image_index);
}

void butter_resize(butter_t *butter, u32 width, u32 height) {
  butter_log_debug("Resizing butter surface window");
  vk_command_pool_create_info_t pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = butter->queue_family;

  vk_result_t res = vkDeviceWaitIdle(butter->device);
  if (res != VK_SUCCESS) {
    butter_log_error("Could not wait for device idle");
    return;
  }

  vkDestroyCommandPool(butter->device, butter->cmd_pool, null);
  if ((res = butter_update_surface(butter, BUTTER_LATENCY_CAP, width,
                                   height)) != VK_SUCCESS)
    butter_log_error("Could not update surface: %d", res);

  if ((res = vkCreateCommandPool(butter->device, &pool_info, null,
                                 &butter->cmd_pool)) != VK_SUCCESS)
    butter_log_error("Could not create command pool: %d", res);

  vk_command_buffer_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = butter->cmd_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = butter->image_count;

  if (!butter->cmds)
    butter->cmds = arena_alloc_zeroed(butter->arena, vk_command_buffer_t,
                                      butter->image_count);

  if ((res = vkAllocateCommandBuffers(butter->device, &alloc_info,
                                      butter->cmds)) != VK_SUCCESS)
    butter_log_error("Could not allocate command buffers");
}

static int render_thread_loop(void *arg) {
  butter_context_t *butter = (butter_context_t *)arg;

  while (atomic_load(&butter->render_running)) {
    mtx_lock(&butter->render_mutex);
    while (!atomic_load(&butter->frame_requested) &&
           atomic_load(&butter->render_running)) {
      cnd_wait(&butter->frame_ready, &butter->render_mutex);
    }

    if (!atomic_load(&butter->render_running)) {
      mtx_unlock(&butter->render_mutex);
      break;
    }

    atomic_store(&butter->frame_requested, false);

    if (butter->resize_pending) {
      u32 width = butter->pending_width;
      u32 height = butter->pending_height;
      butter->resize_pending = false;
      butter_resize(butter, width, height);
    }

    butter_frame_t *frame = butter_begin_frame(butter->render_arena, butter);
    if (frame) {
      if (butter->draw_callback)
        butter->draw_callback(frame->cmd, frame, butter->draw_userdata);

      vk_result_t res = butter_end_frame(butter->render_arena, butter, frame);
      if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        butter->pending_width = butter->extent.width;
        butter->pending_height = butter->extent.height;
        butter->resize_pending = true;
      }
    }

    atomic_store(&butter->frame_completed, true);
    cnd_signal(&butter->frame_done);
    mtx_unlock(&butter->render_mutex);
  }
  return 0;
}
void butter_set_draw_callback(butter_t *butter, butter_draw_callback_t cb,
                              void *userdata) {
  if (!butter)
    return;
  butter->draw_callback = cb;
  butter->draw_userdata = userdata;
}

void butter_start_render_thread(butter_t *butter, arena_t *per_frame_arena) {
  if (!butter)
    return;
  if (butter->render_running)
    return;
  if (!per_frame_arena)
    return;

  butter->render_arena = per_frame_arena;
  atomic_store(&butter->render_running, true);
  atomic_store(&butter->frame_requested, false);
  atomic_store(&butter->frame_completed, false);
  butter->resize_pending = false;

  thrd_create(&butter->render_thread, render_thread_loop, butter);
}

void butter_stop_render_thread(butter_t *butter) {
  if (!butter || !atomic_load(&butter->render_running))
    return;

  mtx_lock(&butter->render_mutex);
  atomic_store(&butter->render_running, false);
  cnd_signal(&butter->frame_ready);
  mtx_unlock(&butter->render_mutex);

  thrd_join(butter->render_thread, null);
  atomic_store(&butter->render_running, false);
}

void butter_request_frame(butter_t *butter) {
  if (!butter || !atomic_load(&butter->render_running))
    return;

  mtx_lock(&butter->render_mutex);
  atomic_store(&butter->frame_requested, true);
  atomic_store(&butter->frame_completed, false);
  cnd_signal(&butter->frame_ready);
  mtx_unlock(&butter->render_mutex);
}

void butter_wait_for_frame(butter_t *butter) {
  if (!butter || !atomic_load(&butter->render_running))
    return;

  mtx_lock(&butter->render_mutex);
  while (!atomic_load(&butter->frame_completed))
    cnd_wait(&butter->frame_done, &butter->render_mutex);
  mtx_unlock(&butter->render_mutex);
}

b32 butter_is_render_thread_running(const butter_t *butter) {
  return butter ? (b32)atomic_load(&butter->render_running) : false;
}

void butter_set_vsync(butter_t *butter, b32 vsync) {
  if (!butter)
    return;
  if (butter->vsync == vsync)
    return;

  mtx_lock(&butter->render_mutex);
  butter->vsync = vsync;
  butter->resize_pending = true;
  butter->pending_width = butter->extent.width;
  butter->pending_height = butter->extent.height;
  mtx_unlock(&butter->render_mutex);

  if (atomic_load(&butter->render_running)) {
    butter_request_frame(butter);
  }
}

void butter_set_target_refresh_rate(butter_t *butter, f32 rate) {
  if (!butter)
    return;
  butter->target_refresh_rate = rate;
}

f32 butter_get_target_refresh_rate(const butter_t *butter) {
  return butter ? butter->target_refresh_rate : 0.0f;
}

void butter_set_pending_resize(butter_t *butter, u32 width, u32 height) {
  if (!butter)
    return;
  mtx_lock(&butter->render_mutex);
  butter->pending_width = width;
  butter->pending_height = height;
  butter->resize_pending = true;
  mtx_unlock(&butter->render_mutex);
}

/***********************************/

#include <threads.h>

#include <butter/internal/init.h>
#include <butter/internal/present.h>
#include <butter/internal/stats.h>
#include <butter/internal/swapchain.h>
#include <butter/internal/types.h>

#include <butter/log.h>
#include <butter/types.h>

#include <butter/graphics/pipeline.h>

#include <butter/render/aa.h>
#include <butter/render/frame.h>
#include <butter/render/pacing.h>

/***********************************/

butter_frame_t *butter_begin_frame(arena_t *arena, butter_t *butter) {
  if (atomic_load(&butter->aa_dirty)) {
    mtx_lock(&butter->aa_mutex);
    atomic_store(&butter->aa_dirty, false);
    if (butter_recreate_render_resources(butter))
      butter_rebuild_pipelines(butter);
    else
      butter_log_error("Could not recreate render resources for AA change");
    mtx_unlock(&butter->aa_mutex);
  }

  vk_extent2d_t extent = butter->extent;
  if (extent.width == 0 || extent.height == 0) {
    butter_log_error("Extent is zero");
    return null;
  }

  butter->dynamic_vbo_offset = 0;
  butter->dynamic_ibo_offset = 0;

  u32 image_index = 0;
  vk_result_t res;
  if ((res = butter_acquire_next_image(butter, &image_index)) != VK_SUCCESS) {
    if (res == VK_TIMEOUT) {
      butter_log_debug("Acquire timed out - skipping frame");
      return null;
    } else if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
      butter_log_debug("Swapchain out of date - resizing");
    else
      butter_log_debug("Acquire error %d - triggering resize", res);

    butter->resize_pending = true;
    butter->pending_width = butter->extent.width;
    butter->pending_height = butter->extent.height;
    return NULL;
  }

  u32 in_flight_frame_slot = butter->in_flight_frame_slot;
  vk_command_buffer_t cmd = butter->cmds[in_flight_frame_slot];
  vk_command_buffer_begin_info_t begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if ((res = vkResetCommandBuffer(cmd, 0)) != VK_SUCCESS)
    butter_log_error("Could not reset command buffer: %d", res);

  if ((res = vkBeginCommandBuffer(cmd, &begin_info)) != VK_SUCCESS) {
    butter_log_error("Could not begin command buffer: %d", res);
    return null;
  }

  butter_stats_begin_frame(butter, cmd, in_flight_frame_slot);

  vk_render_pass_begin_info_t rp_begin = {0};
  rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp_begin.renderPass = butter->render_pass;
  rp_begin.framebuffer = butter->framebuffers[image_index];
  rp_begin.renderArea.extent = extent;
  rp_begin.renderArea.offset = (vk_offset2d_t){0};

  vk_clear_value_t clears[2] = {
      butter->clear_color,
      (vk_clear_value_t){.depthStencil = {1.0f, 0}},
  };

  rp_begin.clearValueCount = butter->enable_depth ? 2 : 1;
  rp_begin.pClearValues = clears;

  vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

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
  vk_result_t res;

  vkCmdEndRenderPass(frame->cmd);

  butter_stats_end_timestamp(butter, frame->cmd, butter->in_flight_frame_slot);

  if ((res = vkEndCommandBuffer(frame->cmd)) != VK_SUCCESS)
    butter_log_error("Could not end command buffer: %d", res);

  u64 cpu_end_ns = get_time_ns();
  f32 cpu_frame_ms = (f32)((f64)(cpu_end_ns - frame->frame_start_ns) / 1e6f);
  butter_stats_record_cpu(butter, cpu_frame_ms);

  res = butter_submit_and_present(butter, frame->cmd, frame->image_index);

  butter_stats_tick_fps(butter, get_time_ns());
  butter_limit_frame_rate(butter, frame->frame_start_ns, get_time_ns());
  return res;
}

void butter_resize(butter_t *butter, u32 width, u32 height) {
  butter_log_debug("Resizing butter surface window");
  vk_result_t res;

  if (butter->available_vulkan_features & BUTTER_FEATURE_TIMELINE_SEMAPHORE) {
    u64 last_value = butter->timeline_value;
    if (last_value > 0) {
      VkSemaphoreWaitInfo wait_info = {0};
      wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
      wait_info.semaphoreCount = 1;
      wait_info.pSemaphores = &butter->timeline_semaphore;
      wait_info.pValues = &last_value;
      if ((res = vkWaitSemaphores(butter->device, &wait_info, UINT64_MAX)) !=
          VK_SUCCESS) {
        butter_log_error("Could not wait for timeline semaphore");
        return;
      }
    }
  } else {
    if ((res = vkDeviceWaitIdle(butter->device)) != VK_SUCCESS) {
      butter_log_error("Could not wait for device idle");
      return;
    }
  }

  vkFreeCommandBuffers(butter->device, butter->cmd_pool,
                       butter->frames_in_flight, butter->cmds);

  if ((res = butter_update_surface(butter, BUTTER_LATENCY_CAP, width,
                                   height)) != VK_SUCCESS)
    butter_log_error("Could not update surface: %d", res);

  vk_command_buffer_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = butter->cmd_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = butter->frames_in_flight;

  if (!butter->cmds)
    butter->cmds = arena_alloc_zeroed(butter->arena, vk_command_buffer_t,
                                      butter->frames_in_flight);

  if ((res = vkAllocateCommandBuffers(butter->device, &alloc_info,
                                      butter->cmds)) != VK_SUCCESS)
    butter_log_error("Could not allocate command buffers");
}

b32 butter_frame_completed(const butter_t *butter) {
  if (!butter)
    return false;

  return (b32)atomic_load(&butter->frame_completed);
}

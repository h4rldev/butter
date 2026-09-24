/***********************************/

#include <threads.h>

#include <butter/internal/init.h>
#include <butter/internal/present.h>
#include <butter/internal/stats.h>
#include <butter/internal/swapchain.h>
#include <butter/internal/texture.h>
#include <butter/internal/types.h>

#include <butter/log.h>
#include <butter/types.h>

#include <butter/graphics/pipeline.h>

#include <butter/render/aa.h>
#include <butter/render/frame.h>
#include <butter/render/pacing.h>
#include <butter/render/target.h>

/***********************************/

static void butter_frame_to_present(butter_context_t *butter,
                                    vk_command_buffer_t cmd, u32 image_index) {
  vk_image_memory_barrier_t barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = 0;
  barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = butter->images[image_index];
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, null, 0,
                       null, 1, &barrier);
}

//
//
//

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
  vk_result_t res = butter_acquire_next_image(butter, &image_index);
  if (res == VK_SUBOPTIMAL_KHR) {
    /* Suboptimal still hands us a valid image; render it and recreate the
       swapchain afterwards. Bailing out abandons the signaled image_available
       semaphore and breaks every following acquire (VUID-01286). */
    butter->resize_pending = true;
    butter->pending_width = butter->extent.width;
    butter->pending_height = butter->extent.height;
    butter->swapchain_dirty = true;
  } else if (res != VK_SUCCESS) {
    if (res == VK_TIMEOUT)
      butter_log_debug("Acquire timed out - skipping frame");
    else if (res == VK_ERROR_OUT_OF_DATE_KHR)
      butter_log_debug("Swapchain out of date - resizing");
    else
      butter_log_debug("Acquire error %d - triggering resize", res);

    if (res != VK_TIMEOUT) {
      butter->resize_pending = true;
      butter->pending_width = butter->extent.width;
      butter->pending_height = butter->extent.height;
      butter->swapchain_dirty = true;
    }
    return null;
  }

  u32 in_flight_frame_slot = butter->in_flight_frame_slot;

  if (butter->effect_pools && in_flight_frame_slot < butter->effect_pool_cap)
    vkResetDescriptorPool(butter->device,
                          butter->effect_pools[in_flight_frame_slot], 0);

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

  butter->pass_framebuffer = butter->framebuffers[image_index];
  butter->pass_image_index = image_index;
  butter->pass_depth = 0;
  butter_pass_begin(butter, null, false, false);

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

  butter_pass_end(butter);
  butter_frame_to_present(butter, frame->cmd, frame->image_index);
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

butter_texture_t *butter_snapshot(butter_t *butter, butter_target_t *dst, u32 x,
                                  u32 y, u32 width, u32 height) {
  if (!butter || !dst) {
    butter_log_error("Invalid arguments for snapshot");
    return null;
  }

  if (butter->pass_depth == 0) {
    butter_log_error("No active pass to snapshot");
    return null;
  }

  if (dst == butter->pass_target) {
    butter_log_error("Cannot snapshot into the active pass target");
    return null;
  }

  if (width == 0)
    width = butter->extent.width - x;
  if (height == 0)
    height = butter->extent.height - y;

  if (width == 0 || height == 0 || x + width > butter->extent.width ||
      y + height > butter->extent.height) {
    butter_log_error("Snapshot region is out of bounds");
    return null;
  }

  if (width > dst->texture.width || height > dst->texture.height) {
    butter_log_error("Snapshot region is larger than the destination target");
    return null;
  }

  vk_command_buffer_t cmd = butter->cmds[butter->in_flight_frame_slot];
  vk_image_t source = butter->images[butter->pass_image_index];
  vk_image_t dest = dst->texture.image;

  butter_pass_end(butter);

  vk_image_memory_barrier_t to[2] = {0};
  to[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  to[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  to[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  to[0].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  to[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  to[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to[0].image = source;
  to[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  to[0].subresourceRange.levelCount = 1;
  to[0].subresourceRange.layerCount = 1;

  to[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  to[1].srcAccessMask = 0;
  to[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  to[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  to[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  to[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  to[1].image = dest;
  to[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  to[1].subresourceRange.levelCount = 1;
  to[1].subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 2,
                       to);

  vk_image_copy_t region = {0};
  region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.srcSubresource.layerCount = 1;
  region.srcOffset = (vk_offset3d_t){(i32)x, (i32)y, 0};
  region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.dstSubresource.layerCount = 1;
  region.dstOffset = (vk_offset3d_t){0, 0, 0};
  region.extent = (vk_extent3d_t){width, height, 1};

  vkCmdCopyImage(cmd, source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dest,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  vk_image_memory_barrier_t back[2] = {0};
  back[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  back[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  back[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  back[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  back[0].newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  back[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  back[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  back[0].image = source;
  back[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  back[0].subresourceRange.levelCount = 1;
  back[0].subresourceRange.layerCount = 1;

  back[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  back[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  back[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  back[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  back[1].newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  back[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  back[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  back[1].image = dest;
  back[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  back[1].subresourceRange.levelCount = 1;
  back[1].subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0, 0, null, 0, null, 2, back);

  butter_pass_begin(butter, butter->pass_target, true, true);
  return &dst->texture;
}

void butter_resize(butter_t *butter, u32 width, u32 height) {
  if (!butter)
    return;

  b32 recreate = butter->swapchain_dirty || width != butter->extent.width ||
                 height != butter->extent.height;
  if (!recreate)
    return;

  butter->swapchain_dirty = false;

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

  if ((res = butter_update_surface(butter, BUTTER_LATENCY_CAP, width,
                                   height)) != VK_SUCCESS)
    butter_log_error("Could not update surface: %d", res);
}

b32 butter_frame_completed(const butter_t *butter) {
  if (!butter)
    return false;

  return (b32)atomic_load(&butter->frame_completed);
}

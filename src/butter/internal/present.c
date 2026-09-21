#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/internal/present.h>
#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/types.h>

/**
 * @brief Present the rendered image.
 * @details Presents the rendered image to the queue.
 *
 * @param context The context.
 * @param image_index The image index.
 *
 * @return The result of the present.
 */
static vk_result_t butter_present(butter_context_t *context, u32 image_index) {
  vk_present_info_khr_t present_info = {0};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &context->rendering_finished[image_index];
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &context->swapchain;
  present_info.pImageIndices = &image_index;

  b32 paced =
      (context->available_vulkan_features & BUTTER_FEATURE_PRESENT_WAIT) &&
      atomic_load(&context->vsync) && context->swapchain_fresh;

  u64 present_id = 0;
  vk_present_id_khr_t present_id_info = {0};
  if (paced) {
    present_id = ++context->present_id;
    present_id_info.sType = VK_STRUCTURE_TYPE_PRESENT_ID_KHR;
    present_id_info.swapchainCount = 1;
    present_id_info.pPresentIds = &present_id;
    present_info.pNext = &present_id_info;
  }

  vk_result_t res = vkQueuePresentKHR(context->queue, &present_info);

  if (paced && (res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR)) {
    vk_result_t wait_res = context->wait_for_present(
        context->device, context->swapchain, present_id, 100000000);
    if (wait_res == VK_TIMEOUT)
      butter_log_warning("Wait for present timed out: %d", wait_res);
    else if (wait_res != VK_SUCCESS)
      butter_log_error("Could not wait for present: %d", wait_res);
  }

  context->swapchain_fresh = false;
  return res;
}
//
//
//

vk_result_t butter_acquire_next_image(butter_context_t *context,
                                      u32 *image_index) {
  vk_result_t res;
  u32 in_flight_frame_slot = context->in_flight_frame_slot;

  b32 timeline = (context->available_vulkan_features &
                  BUTTER_FEATURE_TIMELINE_SEMAPHORE) != 0;

  if (timeline) {
#ifdef VK_API_VERSION_1_2
    i64 wait_value = context->timeline_value - context->frames_in_flight + 1;
    if (wait_value > 0) {
      vk_semaphore_wait_info_t wait_info = {0};
      wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
      wait_info.semaphoreCount = 1;
      wait_info.pSemaphores = &context->timeline_semaphore;
      wait_info.pValues = (u64 *)&wait_value;

      if ((res = vkWaitSemaphores(context->device, &wait_info, 100000000)) !=
          VK_SUCCESS) {
        if (res == VK_TIMEOUT)
          butter_log_warning("Timeline semaphore timed out: %d", res);
        else
          butter_log_error("Could not wait for timeline semaphore: %d", res);
        return res;
      }
    }
#else
    butter_log_fatal(
        "Timeline semaphores requested but Vulkan 1.2 unavailable");
    return VK_ERROR_INITIALIZATION_FAILED;
#endif
  } else {
    if ((res = vkWaitForFences(context->device, 1,
                               &context->in_flight_fences[in_flight_frame_slot],
                               VK_TRUE, 100000000)) != VK_SUCCESS) {
      butter_log_warning("Could not wait for fence: %d", res);
      return res;
    }

    if ((res = vkResetFences(
             context->device, 1,
             &context->in_flight_fences[in_flight_frame_slot])) != VK_SUCCESS)
      butter_log_error("Could not reset fence");
  }

  return vkAcquireNextImageKHR(context->device, context->swapchain, 100000000,
                               context->image_available[in_flight_frame_slot],
                               VK_NULL_HANDLE, image_index);
}

vk_result_t butter_submit_and_present(butter_context_t *context,
                                      vk_command_buffer_t cmd,
                                      u32 image_index) {
  vk_result_t res;
  u32 in_flight_frame_slot = context->in_flight_frame_slot;
  b32 timeline = (context->available_vulkan_features &
                  BUTTER_FEATURE_TIMELINE_SEMAPHORE) != 0;

  vk_pipeline_stage_flags_t wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };

  vk_submit_info_t submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &context->image_available[in_flight_frame_slot];
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd;

  vk_fence_t fence = VK_NULL_HANDLE;
  vk_semaphore_t signal_semaphores[2] = {
      VK_NULL_HANDLE,
      context->rendering_finished[image_index],
  };

  u64 signal_values[2] = {0, 0};
  vk_timeline_semaphore_submit_info_t timeline_info = {0};

  if (timeline) {
#ifdef VK_API_VERSION_1_2
    timeline_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline_info.signalSemaphoreValueCount = 2;
    timeline_info.pSignalSemaphoreValues = signal_values;

    signal_values[0] = context->timeline_value + 1;
    signal_semaphores[0] = context->timeline_semaphore;
    signal_values[1] = 0;

    submit_info.pNext = &timeline_info;
    submit_info.signalSemaphoreCount = 2;
    submit_info.pSignalSemaphores = signal_semaphores;
#else
    butter_log_fatal(
        "Timeline semaphores requested but Vulkan 1.2 unavailable");
    return VK_ERROR_INITIALIZATION_FAILED;
#endif
  } else {
    fence = context->in_flight_fences[in_flight_frame_slot];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context->rendering_finished[image_index];
  }

  if ((res = vkQueueSubmit(context->queue, 1, &submit_info, fence)) !=
      VK_SUCCESS) {
    butter_log_error("Could not submit queue: %d", res);
    return res;
  }

  if (timeline)
    context->timeline_value = signal_values[0];

  res = butter_present(context, image_index);
  context->in_flight_frame_slot =
      (in_flight_frame_slot + 1) % context->frames_in_flight;
  return res;
}

#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/internal/present.h>
#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/types.h>

vk_result_t butter_acquire_next_image(butter_context_t *context,
                                      u32 *image_index) {
  vk_result_t res;
  u32 frame_index = context->frame_index;

  if ((context->available_vulkan_features &
       BUTTER_FEATURE_TIMELINE_SEMAPHORE) == 0) {

    if ((res = vkWaitForFences(context->device, 1,
                               &context->in_flight_fences[frame_index], VK_TRUE,
                               100000000)) != VK_SUCCESS) {
      butter_log_warning("Could not wait for fence: %d", res);
      return res;
    }

    if ((res = vkResetFences(context->device, 1,
                             &context->in_flight_fences[frame_index])) !=
        VK_SUCCESS)
      butter_log_error("Could not reset fence");

    return vkAcquireNextImageKHR(context->device, context->swapchain, 100000000,
                                 context->image_available[frame_index],
                                 VK_NULL_HANDLE, image_index);
  }

#ifdef VK_API_VERSION_1_2
  i64 wait_value = context->timeline_value - context->image_count + 1;
  butter_log_debug(
      "Acquire: timeline_value=%llu, image_count=%u, wait_value=%lld",
      context->timeline_value, context->image_count, wait_value);

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

  u32 image_index_tmp;
  if ((res = vkAcquireNextImageKHR(
           context->device, context->swapchain, 100000000,
           context->image_available[frame_index], VK_NULL_HANDLE,
           &image_index_tmp)) != VK_SUCCESS) {
    return res;
  }

  *image_index = image_index_tmp;
  return VK_SUCCESS;

#else
  butter_log_fatal("How did you get here?");
  return VK_ERROR_INITIALIZATION_FAILED;
#endif
}

vk_result_t butter_submit_and_present(butter_context_t *context,
                                      vk_command_buffer_t cmd,
                                      u32 image_index) {
  vk_result_t res;
  u32 frame_index = context->frame_index;

  vk_pipeline_stage_flags_t wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };

  if ((context->available_vulkan_features &
       BUTTER_FEATURE_TIMELINE_SEMAPHORE) == 0) {

    vk_submit_info_t submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context->image_available[frame_index];
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context->rendering_finished[image_index];

    if ((res = vkQueueSubmit(context->queue, 1, &submit_info,
                             context->in_flight_fences[frame_index])) !=
        VK_SUCCESS) {
      butter_log_error("Could not submit queue");
      return res;
    }

    vk_present_info_khr_t present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &context->rendering_finished[image_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &context->swapchain;
    present_info.pImageIndices = &image_index;

    res = vkQueuePresentKHR(context->queue, &present_info);
    context->frame_index = (frame_index + 1) % context->image_count;
    return res;
  }

#ifdef VK_API_VERSION_1_2
  u64 signal_value = context->timeline_value + 1;
  u64 signal_values[2] = {signal_value, 0};

  vk_timeline_semaphore_submit_info_t timeline_submit_info = {0};
  timeline_submit_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
  timeline_submit_info.signalSemaphoreValueCount = 2;
  timeline_submit_info.pSignalSemaphoreValues = signal_values;

  vk_semaphore_t signal_semaphores[2] = {
      context->timeline_semaphore,
      context->rendering_finished[image_index],
  };

  vk_submit_info_t submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = &timeline_submit_info;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &context->image_available[frame_index];
  submit_info.pWaitDstStageMask = wait_stages;
  submit_info.signalSemaphoreCount = 2;
  submit_info.pSignalSemaphores = signal_semaphores;

  if (!context->queue)
    butter_log_fatal("Queue is null");
  if (!context->image_available[frame_index])
    butter_log_fatal("Image available semaphore is null");
  if (cmd == VK_NULL_HANDLE)
    butter_log_fatal("Command buffer is null");

  if ((res = vkQueueSubmit(context->queue, 1, &submit_info, VK_NULL_HANDLE)) !=
      VK_SUCCESS) {
    butter_log_error("Could not submit queue: %d", res);
    return res;
  }

  context->timeline_value = signal_value;

  vk_present_info_khr_t present_info = {0};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &context->rendering_finished[image_index];
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &context->swapchain;
  present_info.pImageIndices = &image_index;

  res = vkQueuePresentKHR(context->queue, &present_info);
  context->frame_index = (frame_index + 1) % context->image_count;
  return res;
#else
  butter_log_fatal("How did you get here?");
  return VK_ERROR_INITIALIZATION_FAILED;
#endif
}

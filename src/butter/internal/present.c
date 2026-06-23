#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/internal/present.h>
#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/types.h>

vk_result_t butter_acquire_next_image(butter_context_t *context,
                                      u32 *image_index) {
  u32 frame_index = context->frame_index;
  vk_result_t res;

  if ((res = vkWaitForFences(context->device, 1,
                             &context->in_flight_fences[frame_index], VK_TRUE,
                             100000000)) != VK_SUCCESS) {
    butter_log_error("Could not wait for fence");
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

vk_result_t butter_submit_and_present(butter_context_t *context,
                                      vk_command_buffer_t cmd,
                                      u32 image_index) {
  u32 frame_index = context->frame_index;
  vk_result_t res;

  vk_pipeline_stage_flags_t wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };

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

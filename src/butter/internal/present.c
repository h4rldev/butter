#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/internal/present.h>
#include <butter/internal/types.h>
#include <butter/types.h>

vk_result_t butter_acquire_next_image(butter_context_t *context,
                                      u32 *image_index) {
  u32 frame_index = context->frame_index;

  vkWaitForFences(context->device, 1, &context->in_flight_fences[frame_index],
                  VK_TRUE, UINT64_MAX);
  vkResetFences(context->device, 1, &context->in_flight_fences[frame_index]);

  return vkAcquireNextImageKHR(context->device, context->swapchain, UINT64_MAX,
                               context->image_available[frame_index],
                               VK_NULL_HANDLE, image_index);
}

vk_result_t butter_submit_and_present(butter_context_t *context,
                                      vk_command_buffer_t cmd,
                                      u32 image_index) {
  u32 frame_index = context->frame_index;

  vk_pipeline_stage_flags_t wait_stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };

  vk_submit_info_t submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &context->image_available[frame_index],
      .pWaitDstStageMask = wait_stages,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &context->rendering_finished[image_index],
  };

  vk_result_t res = vkQueueSubmit(context->queue, 1, &submit_info,
                                  context->in_flight_fences[frame_index]);
  if (res != VK_SUCCESS)
    return res;

  vk_present_info_khr_t present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &context->rendering_finished[image_index],
      .swapchainCount = 1,
      .pSwapchains = &context->swapchain,
      .pImageIndices = &image_index,
  };

  res = vkQueuePresentKHR(context->queue, &present_info);
  context->frame_index = (frame_index + 1) % context->image_count;
  return res;
}

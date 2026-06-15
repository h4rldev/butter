#include <stdlib.h>
#include <string.h>

#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include "butter_internal.h"
#include <butter/swapchain.h>
#include <butter/types.h>
#include <vulkan/vulkan_core.h>

static void destroy_swapchain_resources(butter_context_t *context) {
  if (context->framebuffers) {
    for (u32 i = 0; i < context->image_count; i++)
      if (context->framebuffers[i])
        vkDestroyFramebuffer(context->device, context->framebuffers[i], null);
    context->framebuffers = null;
  }

  if (context->image_views) {
    for (u32 i = 0; i < context->image_count; i++)
      if (context->image_views[i])
        vkDestroyImageView(context->device, context->image_views[i], null);
    context->image_views = null;
  }

  if (context->render_pass) {
    vkDestroyRenderPass(context->device, context->render_pass, null);
    context->render_pass = VK_NULL_HANDLE;
  }

  if (context->swapchain) {
    vkDestroySwapchainKHR(context->device, context->swapchain, null);
    context->swapchain = VK_NULL_HANDLE;
  }
}

b32 butter_create_swapchain(arena_t *arena, butter_context_t *context) {
  vk_surface_capabilities_khr_t caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device,
                                            context->surface, &caps);

  u32 format_count = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device,
                                       context->surface, &format_count, null);

  vk_surface_format_khr_t *formats =
      arena_alloc(arena, vk_surface_format_khr_t, format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      context->physical_device, context->surface, &format_count, formats);

  context->format = formats[0].format;
  for (u32 i = 0; i < format_count; i++)
    if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      context->format = formats[i].format;
      break;
    }

  context->extent = caps.currentExtent;
  if (context->extent.width == UINT32_MAX) {
    context->extent.width = 800;
    context->extent.height = 600;
    if (context->extent.width > caps.maxImageExtent.width)
      context->extent.width = caps.maxImageExtent.width;
    if (context->extent.height > caps.maxImageExtent.height)
      context->extent.height = caps.maxImageExtent.height;
  }

  u32 image_count = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
    image_count = caps.maxImageCount;

  vk_swapchain_create_info_khr_t swapchain_create_info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = context->surface,
      .minImageCount = image_count,
      .imageFormat = context->format,
      .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
      .imageExtent = context->extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .preTransform = caps.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE,
  };

  if (vkCreateSwapchainKHR(context->device, &swapchain_create_info, null,
                           &context->swapchain) != VK_SUCCESS)
    return false;

  vkGetSwapchainImagesKHR(context->device, context->swapchain, &image_count,
                          null);
  context->image_count = image_count;
  context->images = arena_alloc(arena, vk_image_t, image_count);
  vkGetSwapchainImagesKHR(context->device, context->swapchain, &image_count,
                          context->images);

  context->image_views = arena_alloc(arena, vk_image_view_t, image_count);
  for (u32 i = 0; i < image_count; i++) {
    vk_image_view_create_info_t image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = context->images[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = context->format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1,
    };

    if (vkCreateImageView(context->device, &image_view_create_info, null,
                          &context->image_views[i]) != VK_SUCCESS)
      return false;
  }

  vk_attachment_description_t color_att = {
      .format = context->format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  vk_attachment_reference_t color_ref = {
      0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  vk_subpass_description_t subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_ref,
  };

  vk_render_pass_create_info_t render_pass_create_info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &color_att,
      .subpassCount = 1,
      .pSubpasses = &subpass,
  };

  if (vkCreateRenderPass(context->device, &render_pass_create_info, null,
                         &context->render_pass) != VK_SUCCESS)
    return false;

  context->framebuffers = arena_alloc(arena, vk_framebuffer_t, image_count);
  for (u32 i = 0; i < image_count; i++) {
    vk_framebuffer_create_info_t framebuffer_create_info = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = context->render_pass,
        .attachmentCount = 1,
        .pAttachments = &context->image_views[i],
        .width = context->extent.width,
        .height = context->extent.height,
        .layers = 1,
    };

    if (vkCreateFramebuffer(context->device, &framebuffer_create_info, null,
                            &context->framebuffers[i]) != VK_SUCCESS)
      return false;
  }

  return true;
}

vk_result_t butter_update_surface(arena_t *arena, butter_context_t *context) {
  vkDeviceWaitIdle(context->device);
  destroy_swapchain_resources(context);

  if (!butter_create_swapchain(arena, context))
    return VK_ERROR_OUT_OF_DATE_KHR;

  for (u32 i = 0; i < BUTTER_MAX_FRAMES; i++) {
    vkDestroySemaphore(context->device, context->rendering_finished[i], null);
    vkDestroySemaphore(context->device, context->image_available[i], null);
    vkDestroyFence(context->device, context->in_flight_fences[i], null);
  }

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
  return VK_SUCCESS;
}

vk_format_t butter_get_format(butter_context_t *context) {
  return context->format;
}

vk_extent2d_t butter_get_extent(butter_context_t *context) {
  return context->extent;
}

vk_render_pass_t butter_get_default_render_pass(butter_context_t *context) {
  return context->render_pass;
}

vk_framebuffer_t butter_get_framebuffer(butter_context_t *context,
                                        u32 image_index) {
  if (image_index >= context->image_count)
    return VK_NULL_HANDLE;

  return context->framebuffers[image_index];
}

vk_queue_t butter_get_queue(butter_context_t *context) {
  return context->queue;
}
u32 butter_get_queue_family(butter_context_t *context) {
  return context->queue_family;
}

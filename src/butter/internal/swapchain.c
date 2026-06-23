#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/internal/swapchain.h>
#include <butter/internal/types.h>
#include <butter/log.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void butter_destroy_swapchain_resources(butter_context_t *context) {
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
}

b32 butter_create_swapchain(arena_t *arena, butter_context_t *context,
                            u32 latency_cap, u32 desired_width,
                            u32 desired_height) {
  butter_log_debug("Creating swapchain");

  vk_surface_capabilities_khr_t caps;
  vk_result_t res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      context->physical_device, context->surface, &caps);
  if (res != VK_SUCCESS)
    butter_log_error("Could not get surface capabilities");

  u32 mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      context->physical_device, context->surface, &mode_count, NULL);
  vk_present_mode_khr_t *modes =
      arena_alloc(arena, vk_present_mode_khr_t, mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      context->physical_device, context->surface, &mode_count, modes);

  vk_present_mode_khr_t chosen_mode = VK_PRESENT_MODE_FIFO_KHR; // fallback

  if (context->vsync) {
    for (u32 i = 0; i < mode_count; i++)
      if (modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
        chosen_mode = VK_PRESENT_MODE_FIFO_KHR;
        break;
      }

    if (chosen_mode != VK_PRESENT_MODE_FIFO_KHR && mode_count > 0)
      chosen_mode = modes[0];
  } else {
    for (u32 i = 0; i < mode_count; i++)
      if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        chosen_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }

    if (chosen_mode != VK_PRESENT_MODE_MAILBOX_KHR)
      for (u32 i = 0; i < mode_count; i++)
        if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
          chosen_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
          break;
        }

    if (chosen_mode != VK_PRESENT_MODE_MAILBOX_KHR &&
        chosen_mode != VK_PRESENT_MODE_IMMEDIATE_KHR && mode_count > 0)
      chosen_mode = modes[0];
  }

  u32 format_count = 0;
  res = vkGetPhysicalDeviceSurfaceFormatsKHR(
      context->physical_device, context->surface, &format_count, null);
  if (res != VK_SUCCESS)
    butter_log_error("Could not get surface format count");

  vk_surface_format_khr_t *formats =
      arena_alloc_zeroed(arena, vk_surface_format_khr_t, format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(
      context->physical_device, context->surface, &format_count, formats);
  if (res != VK_SUCCESS)
    butter_log_error("Could not get surface formats");

  context->format = formats[0].format;
  for (u32 i = 0; i < format_count; i++)
    if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      context->format = formats[i].format;
      break;
    }

  if (desired_width > 0 && desired_height > 0) {
    context->extent.width = desired_width;
    context->extent.height = desired_height;
  } else {
    context->extent = caps.currentExtent;
    if (context->extent.width == UINT32_MAX) {
      context->extent.width = 800;
      context->extent.height = 600;
    }
  }

  context->extent.width = MIN(context->extent.width, caps.maxImageExtent.width);
  context->extent.height =
      MIN(context->extent.height, caps.maxImageExtent.height);
  context->extent.width = MAX(context->extent.width, caps.minImageExtent.width);
  context->extent.height =
      MAX(context->extent.height, caps.minImageExtent.height);

  if (context->extent.width == 0)
    context->extent.width = 1;
  if (context->extent.height == 0)
    context->extent.height = 1;

  u32 image_count = caps.minImageCount + 1;
  if (latency_cap > 0 && latency_cap != UINT32_MAX) {
    u32 capped = MIN(image_count, latency_cap);
    image_count = MAX(capped, caps.minImageCount);
  }

  if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
    image_count = caps.maxImageCount;

  vk_swapchain_create_info_khr_t swapchain_create_info = {0};
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.surface = context->surface;
  swapchain_create_info.minImageCount = image_count;
  swapchain_create_info.imageFormat = context->format;
  swapchain_create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  swapchain_create_info.imageExtent = context->extent;
  swapchain_create_info.imageArrayLayers = 1;
  swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchain_create_info.preTransform = caps.currentTransform;
  swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchain_create_info.presentMode = chosen_mode;
  swapchain_create_info.clipped = VK_TRUE;
  swapchain_create_info.oldSwapchain = context->swapchain;

  vk_swapchain_khr_t new_swapchain;
  if ((res = vkCreateSwapchainKHR(context->device, &swapchain_create_info, null,
                                  &new_swapchain)) != VK_SUCCESS) {
    butter_log_error("Could not create swapchain: %d", res);
    return false;
  }

  if (context->swapchain != VK_NULL_HANDLE)
    vkDestroySwapchainKHR(context->device, context->swapchain, null);
  context->swapchain = new_swapchain;

  if ((res = vkGetSwapchainImagesKHR(context->device, context->swapchain,
                                     &image_count, null)) != VK_SUCCESS)
    butter_log_error("Could not get swapchain images count");

  context->image_count = image_count;
  context->images = arena_alloc_zeroed(arena, vk_image_t, image_count);
  if ((res = vkGetSwapchainImagesKHR(context->device, context->swapchain,
                                     &image_count, context->images)) !=
      VK_SUCCESS)
    butter_log_error("Could not get swapchain images");

  context->image_views =
      arena_alloc_zeroed(arena, vk_image_view_t, image_count);

  for (u32 i = 0; i < image_count; i++) {
    vk_image_view_create_info_t image_view_create_info = {0};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = context->images[i];
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = context->format;
    image_view_create_info.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.layerCount = 1;

    if ((res = vkCreateImageView(context->device, &image_view_create_info, null,
                                 &context->image_views[i])) != VK_SUCCESS) {
      butter_log_error("Could not create image view at index %d: %d", i, res);
      return false;
    }
  }

  vk_attachment_description_t color_att = {0};
  color_att.format = context->format;
  color_att.samples = VK_SAMPLE_COUNT_1_BIT;
  color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  vk_attachment_reference_t color_ref = {0};
  color_ref.attachment = 0;
  color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  vk_subpass_description_t subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_ref;

  vk_render_pass_create_info_t render_pass_create_info = {0};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.attachmentCount = 1;
  render_pass_create_info.pAttachments = &color_att;
  render_pass_create_info.subpassCount = 1;
  render_pass_create_info.pSubpasses = &subpass;

  if ((res = vkCreateRenderPass(context->device, &render_pass_create_info, null,
                                &context->render_pass)) != VK_SUCCESS) {
    butter_log_error("Could not create render pass: %d", res);
    return false;
  }

  context->framebuffers =
      arena_alloc_zeroed(arena, vk_framebuffer_t, image_count);
  for (u32 i = 0; i < image_count; i++) {
    vk_framebuffer_create_info_t framebuffer_create_info = {0};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = context->render_pass;
    framebuffer_create_info.attachmentCount = 1;
    framebuffer_create_info.pAttachments = &context->image_views[i];
    framebuffer_create_info.width = context->extent.width;
    framebuffer_create_info.height = context->extent.height;
    framebuffer_create_info.layers = 1;

    if ((res = vkCreateFramebuffer(context->device, &framebuffer_create_info,
                                   null, &context->framebuffers[i])) !=
        VK_SUCCESS) {
      butter_log_error("Could not create framebuffer: %d", res);
      return false;
    }
  }

  return true;
}

vk_result_t butter_update_surface(arena_t *arena, butter_context_t *context,
                                  u32 latency_cap, u32 desired_width,
                                  u32 desired_height) {
  vk_result_t res = vkDeviceWaitIdle(context->device);
  if (res != VK_SUCCESS)
    butter_log_error("Could not wait for device idle");

  butter_destroy_swapchain_resources(context);

  if (!butter_create_swapchain(arena, context, latency_cap, desired_width,
                               desired_height)) {
    butter_log_debug("Could not create swapchain, probably out of date");
    return VK_ERROR_OUT_OF_DATE_KHR;
  }

  for (u32 i = 0; i < context->image_count; i++) {
    vkDestroySemaphore(context->device, context->rendering_finished[i], null);
    vkDestroySemaphore(context->device, context->image_available[i], null);
    vkDestroyFence(context->device, context->in_flight_fences[i], null);
  }

  vk_semaphore_create_info_t semaphore_info = {0};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  vk_fence_create_info_t fence_info = {0};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (u32 i = 0; i < context->image_count; i++) {
    vk_result_t res = vkCreateSemaphore(context->device, &semaphore_info, null,
                                        &context->rendering_finished[i]);
    if (res != VK_SUCCESS)
      butter_log_error("Could not create rendering finished semaphore");
    res = vkCreateSemaphore(context->device, &semaphore_info, null,
                            &context->image_available[i]);
    if (res != VK_SUCCESS)
      butter_log_error("Could not create image available semaphore");
    res = vkCreateFence(context->device, &fence_info, null,
                        &context->in_flight_fences[i]);
    if (res != VK_SUCCESS)
      butter_log_error("Could not create in flight fence");
  }

  context->frame_index = 0;
  return VK_SUCCESS;
}

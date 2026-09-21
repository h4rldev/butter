/***********************************/

#include <threads.h>

#include <vulkan/vulkan.h>

#include <htils/basictypes.h>

#include <butter/internal/aa.h>
#include <butter/internal/memory.h>
#include <butter/internal/swapchain.h>
#include <butter/internal/types.h>

#include <butter/log.h>

/***********************************/

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/**
 * @brief Creates the depth resources for the context.
 * @details Creates the depth images, depth image views, and depth memories, and
 * initializes them.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return true on success, false on error.
 */
static b32 butter_create_depth_resources(butter_context_t *context) {
  context->depth_images =
      arena_alloc_zeroed(context->arena, vk_image_t, context->image_count);
  context->depth_image_views =
      arena_alloc_zeroed(context->arena, vk_image_view_t, context->image_count);
  context->depth_memories = arena_alloc_zeroed(
      context->arena, vk_device_memory_t, context->image_count);

  vk_image_create_info_t image_create_info = {0};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = VK_FORMAT_D32_SFLOAT;
  image_create_info.extent =
      (vk_extent3d_t){context->extent.width, context->extent.height, 1};
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.samples = (vk_sample_count_flag_bits_t)context->aa_samples;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  for (u32 i = 0; i < context->image_count; i++) {
    vk_result_t res;

    if ((res = vkCreateImage(context->device, &image_create_info, null,
                             &context->depth_images[i])) != VK_SUCCESS) {
      butter_log_error("Could not create depth image: %d", res);
      return false;
    }

    vk_memory_requirements_t mem_reqs;
    vkGetImageMemoryRequirements(context->device, context->depth_images[i],
                                 &mem_reqs);

    i32 mem_type = butter_find_memory_type(context->physical_device,
                                           mem_reqs.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mem_type == -1) {
      butter_log_fatal("No device-local memory type for depth image");
      return false;
    }

    vk_memory_allocate_info_t alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = mem_type;

    if ((res = vkAllocateMemory(context->device, &alloc_info, null,
                                &context->depth_memories[i])) != VK_SUCCESS) {
      butter_log_error("Could not allocate depth image memory: %d", res);
      return false;
    }

    if ((res = vkBindImageMemory(context->device, context->depth_images[i],
                                 context->depth_memories[i], 0)) !=
        VK_SUCCESS) {
      butter_log_error("Could not bind depth image memory: %d", res);
      return false;
    }

    vk_image_view_create_info_t image_view_create_info = {0};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = context->depth_images[i];
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = VK_FORMAT_D32_SFLOAT;
    image_view_create_info.subresourceRange = (vk_image_subresource_range_t){0};
    image_view_create_info.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_DEPTH_BIT;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.layerCount = 1;

    if ((res = vkCreateImageView(context->device, &image_view_create_info, null,
                                 &context->depth_image_views[i])) !=
        VK_SUCCESS) {
      butter_log_error("Could not create depth image view: %d", res);
      return false;
    }
  }

  return true;
}

//
//
//

/**
 * @brief Create the multisampled color target.
 * @details Allocates one multisampled color image, view, and memory per
 * swapchain image. A no-op when anti-aliasing is off.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return true on success, false on error.
 */
static b32 butter_create_aa_resources(butter_context_t *context) {
  if (context->aa_samples <= 1)
    return true;

  context->aa_color_images =
      arena_alloc_zeroed(context->arena, vk_image_t, context->image_count);
  context->aa_color_image_views =
      arena_alloc_zeroed(context->arena, vk_image_view_t, context->image_count);
  context->aa_color_memories = arena_alloc_zeroed(
      context->arena, vk_device_memory_t, context->image_count);

  vk_image_create_info_t image_create_info = {0};
  image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_create_info.imageType = VK_IMAGE_TYPE_2D;
  image_create_info.format = context->format;
  image_create_info.extent =
      (vk_extent3d_t){context->extent.width, context->extent.height, 1};
  image_create_info.mipLevels = 1;
  image_create_info.arrayLayers = 1;
  image_create_info.samples = (vk_sample_count_flag_bits_t)context->aa_samples;
  image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_create_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  for (u32 i = 0; i < context->image_count; i++) {
    vk_result_t res;

    if ((res = vkCreateImage(context->device, &image_create_info, null,
                             &context->aa_color_images[i])) != VK_SUCCESS) {
      butter_log_error("Could not create AA color image: %d", res);
      return false;
    }

    vk_memory_requirements_t mem_reqs;
    vkGetImageMemoryRequirements(context->device, context->aa_color_images[i],
                                 &mem_reqs);

    i32 mem_type = butter_find_memory_type(context->physical_device,
                                           mem_reqs.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mem_type == -1) {
      butter_log_fatal("No device-local memory type for AA color image");
      return false;
    }

    vk_memory_allocate_info_t alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_reqs.size;
    alloc_info.memoryTypeIndex = mem_type;

    if ((res = vkAllocateMemory(context->device, &alloc_info, null,
                                &context->aa_color_memories[i])) !=
        VK_SUCCESS) {
      butter_log_error("Could not allocate AA color image memory: %d", res);
      return false;
    }

    if ((res = vkBindImageMemory(context->device, context->aa_color_images[i],
                                 context->aa_color_memories[i], 0)) !=
        VK_SUCCESS) {
      butter_log_error("Could not bind AA color image memory: %d", res);
      return false;
    }

    vk_image_view_create_info_t image_view_create_info = {0};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.image = context->aa_color_images[i];
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = context->format;
    image_view_create_info.subresourceRange.aspectMask =
        VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.layerCount = 1;

    if ((res = vkCreateImageView(context->device, &image_view_create_info, null,
                                 &context->aa_color_image_views[i])) !=
        VK_SUCCESS) {
      butter_log_error("Could not create AA color image view: %d", res);
      return false;
    }
  }

  return true;
}

//
//
//

/**
 * @brief Destroy the multisampled color target.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 */
static void butter_destroy_aa_resources(butter_context_t *context) {
  if (context->aa_color_images) {
    for (u32 i = 0; i < context->image_count; i++)
      if (context->aa_color_images[i] != VK_NULL_HANDLE)
        vkDestroyImage(context->device, context->aa_color_images[i], null);
    context->aa_color_images = null;
  }

  if (context->aa_color_image_views) {
    for (u32 i = 0; i < context->image_count; i++)
      if (context->aa_color_image_views[i] != VK_NULL_HANDLE)
        vkDestroyImageView(context->device, context->aa_color_image_views[i],
                           null);

    context->aa_color_image_views = null;
  }

  if (context->aa_color_memories) {
    for (u32 i = 0; i < context->image_count; i++)
      if (context->aa_color_memories[i] != VK_NULL_HANDLE)
        vkFreeMemory(context->device, context->aa_color_memories[i], null);
    context->aa_color_memories = null;
  }
}

//
//
//

/**
 * @brief Create the context's render pass.
 * @details Builds the render pass for the current sample count: a single color
 * attachment when anti-aliasing is off, or a multisampled color attachment with
 * the swapchain image as its resolve target when it is on. The depth
 * attachment, when enabled, matches the color sample count.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return true on success, false on error.
 */
static b32 butter_create_render_pass(butter_context_t *context) {
  b32 msaa = context->aa_samples > 1;
  vk_sample_count_flag_bits_t samples =
      (vk_sample_count_flag_bits_t)context->aa_samples;

  vk_attachment_description_t atts[3] = {0};
  u32 att_count = 0;

  vk_attachment_description_t color_att = {0};
  color_att.format = context->format;
  color_att.samples = msaa ? samples : VK_SAMPLE_COUNT_1_BIT;
  color_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_att.storeOp =
      msaa ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
  color_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_att.finalLayout = msaa ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                               : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  atts[att_count++] = color_att;

  vk_attachment_reference_t color_ref = {0};
  color_ref.attachment = 0;
  color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  vk_attachment_reference_t resolve_ref = {0};
  if (msaa) {
    vk_attachment_description_t resolve_att = {0};
    resolve_att.format = context->format;
    resolve_att.samples = VK_SAMPLE_COUNT_1_BIT;
    resolve_att.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolve_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resolve_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolve_att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    atts[att_count] = resolve_att;

    resolve_ref.attachment = att_count;
    resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    att_count++;
  }

  vk_attachment_reference_t depth_ref = {0};
  if (context->enable_depth) {
    vk_attachment_description_t depth_att = {0};
    depth_att.format = VK_FORMAT_D32_SFLOAT;
    depth_att.samples = msaa ? samples : VK_SAMPLE_COUNT_1_BIT;
    depth_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_att.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_att.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    atts[att_count] = depth_att;

    depth_ref.attachment = att_count;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    att_count++;
  }

  vk_subpass_description_t subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_ref;
  subpass.pResolveAttachments = msaa ? &resolve_ref : null;
  subpass.pDepthStencilAttachment = context->enable_depth ? &depth_ref : null;

  vk_render_pass_create_info_t render_pass_create_info = {0};
  render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  render_pass_create_info.attachmentCount = att_count;
  render_pass_create_info.pAttachments = atts;
  render_pass_create_info.subpassCount = 1;
  render_pass_create_info.pSubpasses = &subpass;

  vk_result_t res;
  if ((res = vkCreateRenderPass(context->device, &render_pass_create_info, null,
                                &context->render_pass)) != VK_SUCCESS) {
    butter_log_error("Could not create render pass: %d", res);
    return false;
  }

  context->render_pass_samples = context->aa_samples;
  return true;
}

//
//
//

/**
 * @brief Create the swapchain framebuffers.
 * @details One framebuffer per swapchain image, including the multisampled
 * color target (when anti-aliasing is on) and the depth target (when enabled).
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return true on success, false on error.
 */
static b32 butter_create_framebuffers(butter_context_t *context) {
  if (!context->framebuffers)
    context->framebuffers = arena_alloc_zeroed(context->arena, vk_framebuffer_t,
                                               context->image_count);

  for (u32 i = 0; i < context->image_count; i++) {
    b32 msaa = context->aa_samples > 1;

    vk_image_view_t fb_attachments[3];
    u32 attachment_count = 0;
    if (msaa)
      fb_attachments[attachment_count++] = context->aa_color_image_views[i];
    fb_attachments[attachment_count++] = context->image_views[i];
    if (context->enable_depth)
      fb_attachments[attachment_count++] = context->depth_image_views[i];

    vk_framebuffer_create_info_t framebuffer_create_info = {0};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = context->render_pass;
    framebuffer_create_info.attachmentCount = attachment_count;
    framebuffer_create_info.pAttachments = fb_attachments;
    framebuffer_create_info.width = context->extent.width;
    framebuffer_create_info.height = context->extent.height;
    framebuffer_create_info.layers = 1;

    vk_result_t res;
    if ((res = vkCreateFramebuffer(context->device, &framebuffer_create_info,
                                   null, &context->framebuffers[i])) !=
        VK_SUCCESS) {
      butter_log_error("Could not create framebuffer: %d", res);
      return false;
    }
  }

  return true;
}

//
//
//

/**
 * @brief Destroy the swapchain framebuffers.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 */
static void butter_destroy_framebuffers(butter_context_t *context) {
  if (!context->framebuffers)
    return;

  for (u32 i = 0; i < context->image_count; i++)
    if (context->framebuffers[i] != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(context->device, context->framebuffers[i], null);
      context->framebuffers[i] = VK_NULL_HANDLE;
    }
  context->framebuffers = null;
}

//
//
//

/**
 * @brief Destroy the depth target.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 */
static void butter_destroy_depth_resources(butter_context_t *context) {
  if (context->depth_images) {
    for (u32 i = 0; i < context->image_count; i++)
      if (context->depth_images[i] != VK_NULL_HANDLE)
        vkDestroyImage(context->device, context->depth_images[i], null);
    context->depth_images = null;
  }

  if (context->depth_image_views) {
    for (u32 i = 0; i < context->image_count; i++)
      if (context->depth_image_views[i] != VK_NULL_HANDLE)
        vkDestroyImageView(context->device, context->depth_image_views[i],
                           null);
    context->depth_image_views = null;
  }

  if (context->depth_memories) {
    for (u32 i = 0; i < context->image_count; i++)
      if (context->depth_memories[i] != VK_NULL_HANDLE)
        vkFreeMemory(context->device, context->depth_memories[i], null);
    context->depth_memories = null;
  }
}

//
//
//

void butter_destroy_swapchain_resources(butter_context_t *context) {
  butter_destroy_framebuffers(context);
  butter_destroy_aa_resources(context);
  butter_destroy_depth_resources(context);

  if (context->image_views) {
    for (u32 i = 0; i < context->image_count; i++)
      if (context->image_views[i] != VK_NULL_HANDLE) {
        butter_log_debug("Destroying image view %p at index %d",
                         context->image_views, i);
        vkDestroyImageView(context->device, context->image_views[i], null);
        context->image_views[i] = VK_NULL_HANDLE;
      }
    context->image_count = 0;
    context->image_views = null;
  }
}

//
//
//

b32 butter_recreate_render_resources(butter_context_t *context) {
  vk_result_t res = vkDeviceWaitIdle(context->device);
  if (res != VK_SUCCESS)
    butter_log_error("Could not wait for device idle");

  butter_destroy_framebuffers(context);
  butter_destroy_aa_resources(context);
  butter_destroy_depth_resources(context);

  if (context->render_pass) {
    vkDestroyRenderPass(context->device, context->render_pass, null);
    context->render_pass = VK_NULL_HANDLE;
  }

  context->aa_samples = butter_aa_resolve_budgeted(context);
  if (!butter_create_render_pass(context))
    return false;
  if (!butter_create_aa_resources(context))
    return false;
  if (context->enable_depth && !butter_create_depth_resources(context))
    return false;
  if (!butter_create_framebuffers(context))
    return false;

  return true;
}

b32 butter_create_swapchain(butter_context_t *context, u32 latency_cap,
                            u32 desired_width, u32 desired_height) {
  butter_log_debug("Creating swapchain");

  vk_surface_capabilities_khr_t caps;
  vk_result_t res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      context->physical_device, context->surface, &caps);
  if (res != VK_SUCCESS)
    butter_log_error("Could not get surface capabilities");

  vk_present_mode_khr_t chosen_mode = context->available_modes[0]; // fallback
  if (context->vsync) {
    for (u32 i = 0; i < context->available_mode_count; i++)
      if (context->available_modes[i] == VK_PRESENT_MODE_FIFO_KHR) {
        chosen_mode = VK_PRESENT_MODE_FIFO_KHR;
        break;
      }
  } else {
    for (u32 i = 0; i < context->available_mode_count; i++)
      if (context->available_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        chosen_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }

    if (chosen_mode != VK_PRESENT_MODE_MAILBOX_KHR)
      for (u32 i = 0; i < context->available_mode_count; i++)
        if (context->available_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
          chosen_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
          break;
        }
  }

  if (!context->format) {
    u32 format_count = 0;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(
        context->physical_device, context->surface, &format_count, null);
    if (res != VK_SUCCESS)
      butter_log_error("Could not get surface format count");

    vk_surface_format_khr_t *formats = arena_alloc_zeroed(
        context->arena, vk_surface_format_khr_t, format_count);
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
  }

  if (desired_width > 0 && desired_height > 0) {
    context->extent.width = desired_width;
    context->extent.height = desired_height;
  } else {
    context->extent = caps.currentExtent;
    if (context->extent.width == UINT32_MAX ||
        context->extent.height == UINT32_MAX) {
      context->extent.width = 800;
      context->extent.height = 600;
    }
  }

  context->extent.width = MIN(context->extent.width, caps.maxImageExtent.width);
  context->extent.height =
      MIN(context->extent.height, caps.maxImageExtent.height);

  if (context->max_render_width > 0)
    context->extent.width =
        MIN(context->extent.width, context->max_render_width);
  if (context->max_render_height > 0)
    context->extent.height =
        MIN(context->extent.height, context->max_render_height);

  context->extent.width = MAX(context->extent.width, caps.minImageExtent.width);
  context->extent.height =
      MAX(context->extent.height, caps.minImageExtent.height);

  if (context->extent.width == 0)
    context->extent.width = 800;
  if (context->extent.height == 0)
    context->extent.height = 600;

  u32 image_count = caps.minImageCount;
  if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
    image_count = caps.maxImageCount;

  context->image_count = image_count;
  u32 frame_depth = latency_cap > 0 && latency_cap != UINT32_MAX
                        ? MIN(image_count, latency_cap)
                        : image_count;
  context->frames_in_flight = MAX(frame_depth, 1u);

  mtx_lock(&context->aa_mutex);
  context->aa_samples = butter_aa_resolve_budgeted(context);
  mtx_unlock(&context->aa_mutex);

  if (context->render_pass &&
      context->render_pass_samples != context->aa_samples) {
    vkDestroyRenderPass(context->device, context->render_pass, null);
    context->render_pass = VK_NULL_HANDLE;
  }

  vk_swapchain_create_info_khr_t swapchain_create_info = {0};
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.surface = context->surface;
  swapchain_create_info.minImageCount = context->image_count;
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

  context->images = arena_alloc_zeroed(context->arena, vk_image_t, image_count);
  if ((res = vkGetSwapchainImagesKHR(context->device, context->swapchain,
                                     &image_count, context->images)) !=
      VK_SUCCESS)
    butter_log_error("Could not get swapchain images");

  if (!context->image_views)
    context->image_views = arena_alloc_zeroed(context->arena, vk_image_view_t,
                                              context->image_count);

  vk_image_view_create_info_t image_view_create_info = {0};
  image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_create_info.format = context->format;
  image_view_create_info.subresourceRange.aspectMask =
      VK_IMAGE_ASPECT_COLOR_BIT;
  image_view_create_info.subresourceRange.levelCount = 1;
  image_view_create_info.subresourceRange.layerCount = 1;

  for (u32 i = 0; i < context->image_count; i++) {
    image_view_create_info.image = context->images[i];

    butter_log_debug("Creating image view at index %d", i);
    if ((res = vkCreateImageView(context->device, &image_view_create_info, null,
                                 &context->image_views[i])) != VK_SUCCESS) {
      context->image_count = 0;
      context->images = NULL;
      context->framebuffers = NULL;
      butter_log_error("Could not create image view at index %d: %d", i, res);
      return false;
    }
  }

  if (!butter_create_aa_resources(context))
    return false;

  if (context->enable_depth && !butter_create_depth_resources(context))
    return false;

  if (!context->render_pass && !butter_create_render_pass(context))
    return false;

  if (!butter_create_framebuffers(context))
    return false;

  context->swapchain_fresh = true;
  return true;
}

vk_result_t butter_update_surface(butter_context_t *context, u32 latency_cap,
                                  u32 desired_width, u32 desired_height) {
  vk_result_t res = vkDeviceWaitIdle(context->device);
  if (res != VK_SUCCESS)
    butter_log_error("Could not wait for device idle");

#ifdef VK_API_VERSION_1_2
  if (context->available_vulkan_features & BUTTER_FEATURE_TIMELINE_SEMAPHORE) {
    if (context->timeline_semaphore) {
      vkDestroySemaphore(context->device, context->timeline_semaphore, NULL);
      context->timeline_semaphore = VK_NULL_HANDLE;
    }
    vk_semaphore_type_create_info_t type_info = {0};
    type_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    type_info.initialValue = 0;

    vk_semaphore_create_info_t sem_info = {0};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    sem_info.pNext = &type_info;

    vkCreateSemaphore(context->device, &sem_info, NULL,
                      &context->timeline_semaphore);
    context->timeline_value = 0;
    butter_log_debug("Timeline reset: timeline_value=0, image_count=%u",
                     context->image_count);
  }
#endif

  butter_destroy_swapchain_resources(context);

  if (!butter_create_swapchain(context, latency_cap, desired_width,
                               desired_height)) {
    butter_log_debug("Could not create swapchain, probably out of date");
    return VK_ERROR_OUT_OF_DATE_KHR;
  }

  for (u32 i = 0; i < context->image_count; i++) {
    if (context->rendering_finished[i])
      vkDestroySemaphore(context->device, context->rendering_finished[i], null);
  }

  for (u32 i = 0; i < context->frames_in_flight; i++) {
    if (context->image_available[i])
      vkDestroySemaphore(context->device, context->image_available[i], null);
    if ((context->available_vulkan_features &
         BUTTER_FEATURE_TIMELINE_SEMAPHORE) == 0 &&
        context->in_flight_fences[i])
      vkDestroyFence(context->device, context->in_flight_fences[i], null);
  }

  vk_semaphore_create_info_t semaphore_info = {0};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  vk_fence_create_info_t fence_info = {0};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (u32 i = 0; i < context->image_count; i++) {
    if ((res = vkCreateSemaphore(context->device, &semaphore_info, null,
                                 &context->rendering_finished[i])) !=
        VK_SUCCESS)
      butter_log_error("Could not create rendering finished semaphore");
  }

  for (u32 i = 0; i < context->frames_in_flight; i++) {
    if ((res = vkCreateSemaphore(context->device, &semaphore_info, null,
                                 &context->image_available[i])) != VK_SUCCESS)
      butter_log_error("Could not create image available semaphore");

    if ((context->available_vulkan_features &
         BUTTER_FEATURE_TIMELINE_SEMAPHORE) == 0)
      if ((res = vkCreateFence(context->device, &fence_info, null,
                               &context->in_flight_fences[i])) != VK_SUCCESS)
        butter_log_error("Could not create in flight fence");
  }

  context->in_flight_frame_slot = 0;
  return VK_SUCCESS;
}

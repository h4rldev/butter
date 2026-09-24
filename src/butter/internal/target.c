/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/memory.h>
#include <butter/internal/target.h>
#include <butter/internal/texture.h>
#include <butter/internal/types.h>

#include <butter/log.h>
#include <butter/texture.h>
#include <butter/types.h>

/***********************************/

static b32 butter_target_create_msaa(butter_context_t *butter,
                                     struct butter_target *target) {
  vk_image_create_info_t image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = butter->format;
  image_info.extent = (vk_extent3d_t){target->width, target->height, 1};
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = (vk_sample_count_flag_bits_t)butter->aa_samples;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  vk_result_t res;
  if ((res = vkCreateImage(butter->device, &image_info, null,
                           &target->msaa_image)) != VK_SUCCESS) {
    butter_log_error("Could not create target MSAA image: %d", res);
    return false;
  }

  vk_memory_requirements_t mem_reqs;
  vkGetImageMemoryRequirements(butter->device, target->msaa_image, &mem_reqs);
  i32 mem_type =
      butter_find_memory_type(butter->physical_device, mem_reqs.memoryTypeBits,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (mem_type == -1) {
    butter_log_fatal("No device-local memory type for target MSAA");
    return false;
  }

  vk_memory_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = (u32)mem_type;
  if ((res = vkAllocateMemory(butter->device, &alloc_info, null,
                              &target->msaa_memory)) != VK_SUCCESS) {
    butter_log_error("Could not allocate target MSAA memory: %d", res);
    return false;
  }

  if ((res = vkBindImageMemory(butter->device, target->msaa_image,
                               target->msaa_memory, 0)) != VK_SUCCESS) {
    butter_log_error("Could not bind target MSAA memory: %d", res);
    return false;
  }

  vk_image_view_create_info_t view_info = {0};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = target->msaa_image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = butter->format;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.layerCount = 1;
  if ((res = vkCreateImageView(butter->device, &view_info, null,
                               &target->msaa_view)) != VK_SUCCESS) {
    butter_log_error("Could not create target MSAA view: %d", res);
    return false;
  }

  return true;
}

//
//
//

static b32 butter_target_create_depth(butter_context_t *butter,
                                      struct butter_target *target) {
  vk_image_create_info_t image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = VK_FORMAT_D32_SFLOAT;
  image_info.extent = (vk_extent3d_t){target->width, target->height, 1};
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = (vk_sample_count_flag_bits_t)butter->aa_samples;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  vk_result_t res;
  if ((res = vkCreateImage(butter->device, &image_info, null,
                           &target->depth_image)) != VK_SUCCESS) {
    butter_log_error("Could not create target depth image: %d", res);
    return false;
  }

  vk_memory_requirements_t mem_reqs;
  vkGetImageMemoryRequirements(butter->device, target->depth_image, &mem_reqs);
  i32 mem_type =
      butter_find_memory_type(butter->physical_device, mem_reqs.memoryTypeBits,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (mem_type == -1) {
    butter_log_fatal("No device-local memory type for target depth");
    return false;
  }

  vk_memory_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = (u32)mem_type;
  if ((res = vkAllocateMemory(butter->device, &alloc_info, null,
                              &target->depth_memory)) != VK_SUCCESS) {
    butter_log_error("Could not allocate target depth memory: %d", res);
    return false;
  }

  if ((res = vkBindImageMemory(butter->device, target->depth_image,
                               target->depth_memory, 0)) != VK_SUCCESS) {
    butter_log_error("Could not bind target depth memory: %d", res);
    return false;
  }

  vk_image_view_create_info_t view_info = {0};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = target->depth_image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = VK_FORMAT_D32_SFLOAT;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.layerCount = 1;
  if ((res = vkCreateImageView(butter->device, &view_info, null,
                               &target->depth_view)) != VK_SUCCESS) {
    butter_log_error("Could not create target depth view: %d", res);
    return false;
  }

  return true;
}

//
//
//

void butter_target_teardown(butter_context_t *butter,
                            struct butter_target *target) {
  if (target->framebuffer)
    vkDestroyFramebuffer(butter->device, target->framebuffer, null);
  if (target->msaa_view)
    vkDestroyImageView(butter->device, target->msaa_view, null);
  if (target->msaa_image)
    vkDestroyImage(butter->device, target->msaa_image, null);
  if (target->msaa_memory)
    vkFreeMemory(butter->device, target->msaa_memory, null);
  if (target->depth_view)
    vkDestroyImageView(butter->device, target->depth_view, null);
  if (target->depth_image)
    vkDestroyImage(butter->device, target->depth_image, null);
  if (target->depth_memory)
    vkFreeMemory(butter->device, target->depth_memory, null);

  target->framebuffer = VK_NULL_HANDLE;
  target->msaa_view = VK_NULL_HANDLE;
  target->msaa_image = VK_NULL_HANDLE;
  target->msaa_memory = VK_NULL_HANDLE;
  target->depth_view = VK_NULL_HANDLE;
  target->depth_image = VK_NULL_HANDLE;
  target->depth_memory = VK_NULL_HANDLE;

  butter_destroy_texture(butter, &target->texture);
}

//
//
//

b32 butter_target_build(butter_context_t *butter,
                        struct butter_target *target) {
  if (!butter_texture_init_target(
          butter, &target->texture, target->width, target->height,
          butter->format, butter->default_sampler,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
              VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
              VK_IMAGE_USAGE_TRANSFER_DST_BIT))
    return false;

  if (butter->aa_samples > 1 && !butter_target_create_msaa(butter, target)) {
    butter_target_teardown(butter, target);
    return false;
  }

  if (butter->enable_depth && !butter_target_create_depth(butter, target)) {
    butter_target_teardown(butter, target);
    return false;
  }

  vk_image_view_t attachments[3];
  u32 attachment_count = 0;
  if (butter->aa_samples > 1)
    attachments[attachment_count++] = target->msaa_view;
  attachments[attachment_count++] = target->texture.view;
  if (butter->enable_depth)
    attachments[attachment_count++] = target->depth_view;

  vk_framebuffer_create_info_t framebuffer_info = {0};
  framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebuffer_info.renderPass = butter->render_pass_target;
  framebuffer_info.attachmentCount = attachment_count;
  framebuffer_info.pAttachments = attachments;
  framebuffer_info.width = target->width;
  framebuffer_info.height = target->height;
  framebuffer_info.layers = 1;

  vk_result_t res;
  if ((res = vkCreateFramebuffer(butter->device, &framebuffer_info, null,
                                 &target->framebuffer)) != VK_SUCCESS) {
    butter_log_error("Could not create target framebuffer: %d", res);
    butter_target_teardown(butter, target);
    return false;
  }

  target->samples = butter->aa_samples;
  return true;
}

//
//
//

b32 butter_target_rebuild(butter_context_t *butter,
                          struct butter_target *target) {
  u32 width = target->follow_width ? butter->extent.width : target->width;
  u32 height = target->follow_height ? butter->extent.height : target->height;
  if (width == 0 || height == 0)
    return true;

  if (width == target->width && height == target->height &&
      target->samples == butter->aa_samples)
    return true;

  target->width = width;
  target->height = height;

  butter_target_teardown(butter, target);
  return butter_target_build(butter, target);
}

//
//
//

b32 butter_rebuild_targets(butter_context_t *context) {
  for (struct butter_target *target = context->targets; target;
       target = target->next)
    if (!butter_target_rebuild(context, target))
      return false;
  return true;
}

//
//
//

void butter_destroy_targets(butter_context_t *context) {
  for (struct butter_target *target = context->targets; target;
       target = target->next)
    butter_target_teardown(context, target);
  context->targets = null;
}

#include <string.h>

#include <butter/graphics.h>
#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/texture.h>
#include <butter/types.h>
#include <threads.h>

static i32 find_memory_type(vk_physical_device_t physical_device,
                            u32 memory_type_bits,
                            vk_memory_property_flags_t required_properties) {
  vk_physical_device_memory_properties_t mem_props;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

  for (u32 i = 0; i < mem_props.memoryTypeCount; i++) {
    if ((memory_type_bits & (1 << i)) &&
        (mem_props.memoryTypes[i].propertyFlags & required_properties) ==
            required_properties) {
      return i;
    }
  }
  butter_log_fatal("Failed to find suitable memory type");
  return -1;
}

butter_texture_t butter_create_texture(butter_t *butter, u32 width, u32 height,
                                       vk_format_t format, const void *data,
                                       u64 data_size, vk_sampler_t sampler) {
  if (!butter) {
    butter_log_error("Butter instance not initialized");
    return (butter_texture_t){0};
  }

  if (!data) {
    butter_log_error("No data provided");
    return (butter_texture_t){0};
  }

  if (data_size == 0) {
    butter_log_error("No data size provided");
    return (butter_texture_t){0};
  }

  if (width == 0 || height == 0) {
    butter_log_error("Invalid texture dimensions");
    return (butter_texture_t){0};
  }

  if (format == VK_FORMAT_UNDEFINED) {
    butter_log_error("Invalid texture format");
    return (butter_texture_t){0};
  }

  if (sampler == VK_NULL_HANDLE) {
    butter_log_error("Invalid sampler");
    return (butter_texture_t){0};
  }

  butter_texture_t texture = {0};
  texture.width = width;
  texture.height = height;
  texture.format = format;
  texture.sampler = sampler;

  vk_image_create_info_t image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = format;
  image_info.extent = (vk_extent3d_t){width, height, 1};
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  vk_result_t res;
  if ((res = vkCreateImage(butter->device, &image_info, null,
                           &texture.image)) != VK_SUCCESS) {
    butter_log_error("Could not create image: %d", res);
    return texture;
  }

  vk_memory_requirements_t mem_reqs;
  vkGetImageMemoryRequirements(butter->device, texture.image, &mem_reqs);
  i32 mem_type =
      find_memory_type(butter->physical_device, mem_reqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (mem_type == -1) {
    butter_log_error("No device-local memory type for texture");
    vkDestroyImage(butter->device, texture.image, NULL);
    return texture;
  }

  vk_memory_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = mem_type;

  if ((res = vkAllocateMemory(butter->device, &alloc_info, null,
                              &texture.memory)) != VK_SUCCESS) {
    butter_log_error("Could not allocate memory for texture: %d", res);
    vkDestroyImage(butter->device, texture.image, null);
    return texture;
  }

  vkBindImageMemory(butter->device, texture.image, texture.memory, 0);

  butter_buffer_t staging_buffer = butter_create_buffer(
      butter, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
  if (staging_buffer.handle == VK_NULL_HANDLE) {
    butter_log_error("Could not create staging buffer");
    vkDestroyImage(butter->device, texture.image, null);
    vkFreeMemory(butter->device, texture.memory, null);
    return texture;
  }

  memcpy(staging_buffer.mapped, data, data_size);

  vk_command_buffer_t cmd;
  vk_command_buffer_allocate_info_t cmd_alloc_info = {0};
  cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_alloc_info.commandPool = butter->upload_pool_sync;
  cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd_alloc_info.commandBufferCount = 1;

  if ((res = vkAllocateCommandBuffers(butter->device, &cmd_alloc_info, &cmd)) !=
      VK_SUCCESS) {
    butter_log_error("Could not allocate command buffer: %d", res);
    butter_destroy_buffer(butter, &staging_buffer);
    vkDestroyImage(butter->device, texture.image, null);
    vkFreeMemory(butter->device, texture.memory, null);
    return texture;
  }

  vk_command_buffer_begin_info_t begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &begin_info);

  if ((butter->available_vulkan_features & BUTTER_FEATURE_SYNCHRONIZATION_2) ==
      0) {
    vk_image_memory_barrier_t barrier_to_dst = {0};
    barrier_to_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier_to_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier_to_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier_to_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier_to_dst.image = texture.image;
    barrier_to_dst.subresourceRange = (vk_image_subresource_range_t){0};
    barrier_to_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier_to_dst.subresourceRange.levelCount = 1;
    barrier_to_dst.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null, 1,
                         &barrier_to_dst);
  } else {
#ifdef VK_API_VERSION_1_3
    vk_image_memory_barrier2_t barrier_to_dst = {0};
    barrier_to_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier_to_dst.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
    barrier_to_dst.srcAccessMask = 0;
    barrier_to_dst.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrier_to_dst.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier_to_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier_to_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier_to_dst.image = texture.image;
    barrier_to_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier_to_dst.subresourceRange.levelCount = 1;
    barrier_to_dst.subresourceRange.layerCount = 1;

    vk_dependency_info_t dependency_info = {0};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &barrier_to_dst;
    vkCmdPipelineBarrier2(cmd, &dependency_info);
#else
    butter_log_fatal("How did you get here?");
    return texture;
#endif
  }

  vk_buffer_image_copy_t region = {0};
  region.bufferOffset = 0;
  region.bufferRowLength = width;
  region.bufferImageHeight = height;
  region.imageSubresource = (vk_image_subresource_layers_t){0};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = (vk_offset3d_t){0, 0, 0};
  region.imageExtent = (vk_extent3d_t){width, height, 1};

  vkCmdCopyBufferToImage(cmd, staging_buffer.handle, texture.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  if ((butter->available_vulkan_features & BUTTER_FEATURE_SYNCHRONIZATION_2) ==
      0) {

    vk_image_memory_barrier_t barrier_to_shader = {0};
    barrier_to_shader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier_to_shader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier_to_shader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier_to_shader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier_to_shader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier_to_shader.image = texture.image;
    barrier_to_shader.subresourceRange = (vk_image_subresource_range_t){0};
    barrier_to_shader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier_to_shader.subresourceRange.levelCount = 1;
    barrier_to_shader.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, null, 0,
                         null, 1, &barrier_to_shader);

  } else {
#ifdef VK_API_VERSION_1_3
    vk_image_memory_barrier2_t barrier_to_shader = {0};
    barrier_to_shader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier_to_shader.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
    barrier_to_shader.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier_to_shader.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
    barrier_to_shader.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
    barrier_to_shader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier_to_shader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier_to_shader.image = texture.image;
    barrier_to_shader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier_to_shader.subresourceRange.levelCount = 1;
    barrier_to_shader.subresourceRange.layerCount = 1;

    vk_dependency_info_t dependency_info = {0};
    dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency_info.imageMemoryBarrierCount = 1;
    dependency_info.pImageMemoryBarriers = &barrier_to_shader;
    vkCmdPipelineBarrier2(cmd, &dependency_info);
#else
    butter_log_fatal("How did you get here?");
    return texture;
#endif
  }

  vkEndCommandBuffer(cmd);

  vk_fence_t fence;
  vk_fence_create_info_t fence_info = {0};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  if ((res = vkCreateFence(butter->device, &fence_info, null, &fence)) !=
      VK_SUCCESS) {
    butter_log_error("Could not create fence: %d", res);
    butter_destroy_buffer(butter, &staging_buffer);
    vkDestroyImage(butter->device, texture.image, null);
    vkFreeCommandBuffers(butter->device, butter->upload_pool_sync, 1, &cmd);
    vkFreeMemory(butter->device, texture.memory, null);
    return texture;
  }

  vk_submit_info_t submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd;

  vkQueueSubmit(butter->queue, 1, &submit_info, fence);
  if ((res = vkWaitForFences(butter->device, 1, &fence, true, 1000000000)) !=
      VK_SUCCESS) {
    if (res == VK_TIMEOUT)
      butter_log_error("Timeout waiting for fence, try again");
    else
      butter_log_error("Could not wait for fence: %d", res);

    butter_destroy_buffer(butter, &staging_buffer);
    vkDestroyImage(butter->device, texture.image, null);
    vkDestroyFence(butter->device, fence, null);
    vkFreeCommandBuffers(butter->device, butter->upload_pool_sync, 1, &cmd);
    vkFreeMemory(butter->device, texture.memory, null);
    return texture;
  }

  vkDestroyFence(butter->device, fence, null);
  vkFreeCommandBuffers(butter->device, butter->upload_pool_sync, 1, &cmd);
  butter_destroy_buffer(butter, &staging_buffer);

  vk_image_view_create_info_t image_view_info = {0};
  image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_info.image = texture.image;
  image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_info.format = format;
  image_view_info.subresourceRange = (vk_image_subresource_range_t){0};
  image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_view_info.subresourceRange.levelCount = 1;
  image_view_info.subresourceRange.layerCount = 1;

  if ((res = vkCreateImageView(butter->device, &image_view_info, null,
                               &texture.view)) != VK_SUCCESS) {
    butter_log_error("Could not create image view: %d", res);
    vkDestroyImage(butter->device, texture.image, null);
    vkFreeMemory(butter->device, texture.memory, null);
    return texture;
  }

  butter_descriptor_set_t descriptor_set =
      butter_allocate_descriptor_set(butter, butter->texture_descriptor_pool,
                                     butter->texture_descriptor_set_layout);

  if (descriptor_set.set == VK_NULL_HANDLE) {
    butter_log_error("Could not allocate descriptor set");
    vkDestroyImage(butter->device, texture.image, null);
    vkFreeMemory(butter->device, texture.memory, null);
    vkDestroyImageView(butter->device, texture.view, null);
    return texture;
  }

  butter_update_descriptor_image(butter, &descriptor_set, 0, texture.view,
                                 texture.sampler);
  texture.descriptor_set = descriptor_set;

  return texture;
}

void butter_destroy_texture(butter_t *butter, butter_texture_t *texture) {
  if (!texture || !butter)
    return;

  butter_log_debug("Deleting View if available");
  if (texture->view)
    vkDestroyImageView(butter->device, texture->view, null);

  butter_log_debug("Deleting Image if available");
  if (texture->image)
    vkDestroyImage(butter->device, texture->image, null);

  butter_log_debug("Deleting Memory if available");
  if (texture->memory)
    vkFreeMemory(butter->device, texture->memory, null);

  texture->view = VK_NULL_HANDLE;
  texture->image = VK_NULL_HANDLE;
  texture->memory = VK_NULL_HANDLE;
}

static int butter_upload_thread(void *userdata) {
  butter_t *butter = (butter_t *)userdata;
  vk_result_t res;

  while (butter->upload_thread_running) {
    mtx_lock(&butter->upload_mutex);
    while (butter->upload_queue_head == butter->upload_queue_tail &&
           butter->upload_thread_running)
      cnd_wait(&butter->upload_ready, &butter->upload_mutex);
    if (!butter->upload_thread_running) {
      mtx_unlock(&butter->upload_mutex);
      break;
    }

    butter_upload_t *upload = &butter->upload_queue[butter->upload_queue_head];
    butter->upload_queue_head =
        (butter->upload_queue_head + 1) % butter->upload_queue_cap;

    mtx_unlock(&butter->upload_mutex);

    if (upload->cancelled || atomic_load(&upload->texture.upload_cancelled)) {
      butter_destroy_buffer(butter, &upload->staging_buffer);
      atomic_store(&upload->texture.upload_failed, true);
      atomic_store(&upload->texture.upload_ready, false);
      atomic_store(&upload->texture.upload_cancelled, false);
      continue;
    }

    vk_command_buffer_t cmd;
    vk_command_buffer_allocate_info_t alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = butter->upload_pool_async;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if ((res = vkAllocateCommandBuffers(butter->device, &alloc_info, &cmd)) !=
        VK_SUCCESS) {
      mtx_lock(&butter->upload_mutex);
      butter_log_error("Could not allocate upload command buffer: %d", res);
      butter_destroy_buffer(butter, &upload->staging_buffer);
      vkDestroyImage(butter->device, upload->texture.image, null);
      vkDestroyImageView(butter->device, upload->texture.view, null);
      vkFreeMemory(butter->device, upload->texture.memory, null);
      upload->failed = true;
      atomic_store(&upload->texture.upload_failed, true);
      mtx_unlock(&butter->upload_mutex);
      continue;
    }

    vk_command_buffer_begin_info_t begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_info);

    if ((butter->available_vulkan_features &
         BUTTER_FEATURE_SYNCHRONIZATION_2) == 0) {
      vk_image_memory_barrier_t barrier_to_dst = {0};
      barrier_to_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier_to_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barrier_to_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier_to_dst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier_to_dst.image = upload->texture.image;
      barrier_to_dst.subresourceRange = (vk_image_subresource_range_t){0};
      barrier_to_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier_to_dst.subresourceRange.levelCount = 1;
      barrier_to_dst.subresourceRange.layerCount = 1;

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, null, 0, null,
                           1, &barrier_to_dst);
    } else {
#ifdef VK_API_VERSION_1_3
      vk_image_memory_barrier2_t barrier_to_dst = {0};
      barrier_to_dst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
      barrier_to_dst.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
      barrier_to_dst.srcAccessMask = 0;
      barrier_to_dst.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
      barrier_to_dst.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
      barrier_to_dst.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barrier_to_dst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier_to_dst.image = upload->texture.image;
      barrier_to_dst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier_to_dst.subresourceRange.levelCount = 1;
      barrier_to_dst.subresourceRange.layerCount = 1;

      vk_dependency_info_t dependency_info = {0};
      dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
      dependency_info.imageMemoryBarrierCount = 1;
      dependency_info.pImageMemoryBarriers = &barrier_to_dst;
      vkCmdPipelineBarrier2(cmd, &dependency_info);
#else
      mtx_lock(&butter->upload_mutex);
      butter_log_fatal("How did you get here?");
      butter_destroy_buffer(butter, &upload->staging_buffer);
      vkDestroyImage(butter->device, upload->texture.image, null);
      vkDestroyImageView(butter->device, upload->texture.view, null);
      vkFreeCommandBuffers(butter->device, butter->upload_pool_async, 1, &cmd);
      vkFreeMemory(butter->device, upload->texture.memory, null);

      upload->failed = true;
      atomic_store(&upload->texture.upload_failed, true);
      mtx_unlock(&butter->upload_mutex);
      return texture;
#endif
    }

    vk_buffer_image_copy_t region = {0};
    region.bufferOffset = 0;
    region.bufferRowLength = upload->texture.width;
    region.bufferImageHeight = upload->texture.height;
    region.imageSubresource = (vk_image_subresource_layers_t){0};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = (vk_offset3d_t){0, 0, 0};
    region.imageExtent =
        (vk_extent3d_t){upload->texture.width, upload->texture.height, 1};

    vkCmdCopyBufferToImage(cmd, upload->staging_buffer.handle,
                           upload->texture.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if ((butter->available_vulkan_features &
         BUTTER_FEATURE_SYNCHRONIZATION_2) == 0) {
      vk_image_memory_barrier_t barrier_to_shader = {0};
      barrier_to_shader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier_to_shader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier_to_shader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier_to_shader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barrier_to_shader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      barrier_to_shader.image = upload->texture.image;
      barrier_to_shader.subresourceRange = (vk_image_subresource_range_t){0};
      barrier_to_shader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier_to_shader.subresourceRange.levelCount = 1;
      barrier_to_shader.subresourceRange.layerCount = 1;

      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, null, 0,
                           null, 1, &barrier_to_shader);
    } else {
#ifdef VK_API_VERSION_1_3
      vk_image_memory_barrier2_t barrier_to_shader = {0};
      barrier_to_shader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
      barrier_to_shader.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
      barrier_to_shader.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
      barrier_to_shader.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
      barrier_to_shader.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
      barrier_to_shader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barrier_to_shader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier_to_shader.image = upload->texture.image;
      barrier_to_shader.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier_to_shader.subresourceRange.levelCount = 1;
      barrier_to_shader.subresourceRange.layerCount = 1;

      vk_dependency_info_t dependency_info = {0};
      dependency_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
      dependency_info.imageMemoryBarrierCount = 1;
      dependency_info.pImageMemoryBarriers = &barrier_to_shader;
      vkCmdPipelineBarrier2(cmd, &dependency_info);
#else
      mtx_lock(&butter->upload_mutex);
      butter_log_fatal("How did you get here?");
      butter_destroy_buffer(butter, &upload->staging_buffer);
      vkDestroyImage(butter->device, upload->texture.image, null);
      vkDestroyImageView(butter->device, upload->texture.view, null);
      vkFreeCommandBuffers(butter->device, butter->upload_pool_async, 1, &cmd);
      vkFreeMemory(butter->device, upload->texture.memory, null);

      upload->failed = true;
      atomic_store(&upload->texture.upload_failed, true);
      mtx_unlock(&butter->upload_mutex);
      return texture;
#endif
    }

    vkEndCommandBuffer(cmd);

    butter_descriptor_set_t descriptor_set =
        butter_allocate_descriptor_set(butter, butter->texture_descriptor_pool,
                                       butter->texture_descriptor_set_layout);
    if (descriptor_set.set == VK_NULL_HANDLE) {
      mtx_lock(&butter->upload_mutex);
      butter_log_error("Could not allocate descriptor set");
      butter_destroy_buffer(butter, &upload->staging_buffer);
      vkDestroyImage(butter->device, upload->texture.image, null);
      vkDestroyImageView(butter->device, upload->texture.view, null);
      vkFreeCommandBuffers(butter->device, butter->upload_pool_async, 1, &cmd);
      vkFreeMemory(butter->device, upload->texture.memory, null);

      upload->failed = true;
      atomic_store(&upload->texture.upload_failed, true);
      mtx_unlock(&butter->upload_mutex);
      continue;
    }

    butter_update_descriptor_image(butter, &descriptor_set, 0,
                                   upload->texture.view,
                                   upload->texture.sampler);

    upload->texture.descriptor_set = descriptor_set;

    vk_fence_t fence;
    vk_fence_create_info_t fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    if ((res = vkCreateFence(butter->device, &fence_info, null, &fence)) !=
        VK_SUCCESS) {
      mtx_lock(&butter->upload_mutex);
      butter_log_error("Could not create fence: %d", res);
      butter_destroy_buffer(butter, &upload->staging_buffer);
      vkDestroyImage(butter->device, upload->texture.image, null);
      vkDestroyImageView(butter->device, upload->texture.view, null);
      vkFreeCommandBuffers(butter->device, butter->upload_pool_async, 1, &cmd);
      vkFreeMemory(butter->device, upload->texture.memory, null);

      upload->failed = true;
      atomic_store(&upload->texture.upload_failed, true);
      mtx_unlock(&butter->upload_mutex);
      continue;
    }

    vk_submit_info_t submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;

    vkQueueSubmit(butter->queue, 1, &submit_info, fence);
    if ((res = vkWaitForFences(butter->device, 1, &fence, true, 1000000000)) !=
        VK_SUCCESS) {
      mtx_lock(&butter->upload_mutex);

      if (res == VK_TIMEOUT)
        butter_log_error("Timeout waiting for fence, try again");
      else
        butter_log_error("Could not wait for fence: %d", res);

      butter_destroy_buffer(butter, &upload->staging_buffer);
      vkDestroyImage(butter->device, upload->texture.image, null);
      vkDestroyImageView(butter->device, upload->texture.view, null);
      vkDestroyFence(butter->device, fence, null);
      vkFreeCommandBuffers(butter->device, butter->upload_pool_async, 1, &cmd);
      vkFreeMemory(butter->device, upload->texture.memory, null);
      upload->failed = true;
      atomic_store(&upload->texture.upload_failed, true);
      mtx_unlock(&butter->upload_mutex);
      continue;
    }

    mtx_lock(&butter->upload_mutex);

    vkDestroyFence(butter->device, fence, null);
    vkFreeCommandBuffers(butter->device, butter->upload_pool_async, 1, &cmd);
    butter_destroy_buffer(butter, &upload->staging_buffer);

    upload->ready = true;
    atomic_store(&upload->texture.upload_ready, true);
    atomic_store(&upload->texture.upload_failed, false);
    atomic_store(&upload->texture.upload_cancelled, false);
    mtx_unlock(&butter->upload_mutex);
  }

  return 0;
}

void butter_init_texture_upload(butter_t *butter, u32 queue_cap) {
  vk_command_pool_create_info_t pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = butter->queue_family;
  pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

  vk_result_t res;

  if ((res = vkCreateCommandPool(butter->device, &pool_info, null,
                                 &butter->upload_pool_async)) != VK_SUCCESS) {
    butter_log_error("Could not create async upload pool: %d", res);
    return;
  }

  butter->upload_queue =
      arena_alloc_zeroed(butter->arena, butter_upload_t, queue_cap);
  butter->upload_queue_cap = queue_cap;
  butter->upload_queue_head = 0;
  butter->upload_queue_tail = 0;

  mtx_init(&butter->upload_mutex, mtx_plain);
  cnd_init(&butter->upload_ready);
  butter->upload_thread_running = true;
  thrd_create(&butter->upload_thread, butter_upload_thread, butter);
}

void butter_stop_texture_uploads(butter_t *butter) {
  mtx_lock(&butter->upload_mutex);
  butter->upload_thread_running = false;
  cnd_signal(&butter->upload_ready);
  mtx_unlock(&butter->upload_mutex);

  thrd_join(butter->upload_thread, null);

  mtx_destroy(&butter->upload_mutex);
  cnd_destroy(&butter->upload_ready);
}

butter_texture_t butter_submit_texture_upload(butter_t *butter, u32 width,
                                              u32 height, vk_format_t format,
                                              const void *data, u64 data_size,
                                              vk_sampler_t sampler) {
  butter_texture_t texture = {0};
  texture.width = width;
  texture.height = height;
  texture.format = format;
  texture.sampler = sampler;
  atomic_store(&texture.is_upload, true);
  atomic_store(&texture.upload_ready, false);
  atomic_store(&texture.upload_failed, false);
  atomic_store(&texture.upload_cancelled, false);

  vk_image_create_info_t image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = format;
  image_info.extent = (vk_extent3d_t){width, height, 1};
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  vk_result_t res;
  if ((res = vkCreateImage(butter->device, &image_info, null,
                           &texture.image)) != VK_SUCCESS) {
    butter_log_error("Could not create image: %d", res);
    return texture;
  }

  vk_memory_requirements_t mem_reqs;
  vkGetImageMemoryRequirements(butter->device, texture.image, &mem_reqs);
  i32 mem_type =
      find_memory_type(butter->physical_device, mem_reqs.memoryTypeBits,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (mem_type == -1) {
    butter_log_error("No device-local memory type for texture");
    vkDestroyImage(butter->device, texture.image, NULL);
    return texture;
  }

  vk_memory_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = mem_type;

  if ((res = vkAllocateMemory(butter->device, &alloc_info, null,
                              &texture.memory)) != VK_SUCCESS) {
    butter_log_error("Could not allocate memory for texture: %d", res);
    vkDestroyImage(butter->device, texture.image, null);
    return texture;
  }

  vkBindImageMemory(butter->device, texture.image, texture.memory, 0);

  butter_buffer_t staging_buffer = butter_create_buffer(
      butter, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
  if (staging_buffer.handle == VK_NULL_HANDLE) {
    butter_log_error("Could not create staging buffer");
    vkDestroyImage(butter->device, texture.image, null);
    vkFreeMemory(butter->device, texture.memory, null);
    return texture;
  }

  memcpy(staging_buffer.mapped, data, data_size);

  vk_image_view_create_info_t image_view_info = {0};
  image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_info.image = texture.image;
  image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_info.format = format;
  image_view_info.subresourceRange = (vk_image_subresource_range_t){0};
  image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_view_info.subresourceRange.levelCount = 1;
  image_view_info.subresourceRange.layerCount = 1;

  if ((res = vkCreateImageView(butter->device, &image_view_info, null,
                               &texture.view)) != VK_SUCCESS) {
    butter_log_error("Could not create image view: %d", res);
    vkDestroyImage(butter->device, texture.image, null);
    vkFreeMemory(butter->device, texture.memory, null);
    return texture;
  }

  mtx_lock(&butter->upload_mutex);

  u32 tail = butter->upload_queue_tail;
  u32 next_tail = (tail + 1) % butter->upload_queue_cap;

  if (next_tail == butter->upload_queue_head) {
    butter_log_error("Upload queue full");
    mtx_unlock(&butter->upload_mutex);
    butter_destroy_texture(butter, &texture);
    return (butter_texture_t){0};
  }

  butter_upload_t *upload = &butter->upload_queue[tail];
  upload->texture = texture;
  upload->staging_buffer = staging_buffer;
  upload->ready = false;
  upload->failed = false;
  butter->upload_queue_tail = next_tail;

  cnd_signal(&butter->upload_ready);
  mtx_unlock(&butter->upload_mutex);

  return texture;
}

void butter_stop_texture_upload(butter_t *butter, butter_texture_t *texture) {
  if (!butter || !texture)
    return;

  mtx_lock(&butter->upload_mutex);

  u32 idx = butter->upload_queue_head;
  while (idx != butter->upload_queue_tail) {
    butter_upload_t *upload = &butter->upload_queue[idx];

    if (upload->texture.image == texture->image) {
      upload->cancelled = true;
      atomic_store(&upload->texture.upload_cancelled, true);
      butter_log_debug("Cancelled upload for texture (image=%p)",
                       (void *)texture->image);
      break;
    }

    idx = (idx + 1) % butter->upload_queue_cap;
  }

  mtx_unlock(&butter->upload_mutex);
}

b32 butter_texture_is_ready(const butter_texture_t *texture) {
  b32 is_upload = atomic_load(&texture->is_upload);
  b32 upload_ready = atomic_load(&texture->upload_ready);
  b32 upload_failed = atomic_load(&texture->upload_failed);

  return is_upload && upload_ready && !upload_failed;
}

i32 butter_texture_register(butter_t *butter, butter_texture_t texture) {
  if (butter->texture_registry.count >= butter->texture_registry.capacity) {
    butter_log_error("Texture registry is full");
    return -1;
  }

  u32 id = butter->texture_registry.next_id++;
  u32 index = butter->texture_registry.count;
  butter->texture_registry.entries[index].id = id;
  butter->texture_registry.entries[index].texture = texture;
  butter->texture_registry.count++;

  return id;
}

butter_texture_t *butter_texture_get(butter_t *butter, i32 id) {
  if (id < 0) {
    butter_log_error("Invalid texture ID");
    return null;
  }

  for (u32 i = 0; i < butter->texture_registry.count; i++) {
    if (butter->texture_registry.entries[i].id == (u32)id)
      return &butter->texture_registry.entries[i].texture;
  }

  butter_log_warning("Can't get texture: Texture ID not found");
  return null;
}

void butter_texture_deregister(butter_t *butter, i32 id) {
  if (id < 0) {
    butter_log_error("Invalid texture ID");
    return;
  }

  if (id == 0) {
    butter_log_warning("Can't deregister texture 0");
    return;
  }

  for (u32 i = 0; i < butter->texture_registry.count; i++) {
    if (butter->texture_registry.entries[i].id == (u32)id) {
      butter_texture_t *texture = &butter->texture_registry.entries[i].texture;

      butter_stop_texture_upload(butter, texture);
      butter_destroy_texture(butter, texture);

      butter->texture_registry.entries[i] =
          butter->texture_registry.entries[butter->texture_registry.count - 1];
      butter->texture_registry.count--;
      return;
    }
  }

  butter_log_warning("Can't deregister: Texture ID not found");
}

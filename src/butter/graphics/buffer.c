/***********************************/

#include <string.h>

#include <htils/basictypes.h>

#include <butter/internal/memory.h>
#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/types.h>

/***********************************/

butter_buffer_t butter_create_buffer(butter_t *butter, u64 size,
                                     vk_buffer_usage_flags_t usage,
                                     b32 host_visible) {
  vk_result_t res;

  vk_buffer_create_info_t buffer_info = {0};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  vk_buffer_t buffer;
  if ((res = vkCreateBuffer(butter->device, &buffer_info, null, &buffer)) !=
      VK_SUCCESS) {
    butter_log_error("Could not create buffer: %d", res);
    return (butter_buffer_t){0};
  }

  vk_memory_requirements_t mem_reqs;
  vkGetBufferMemoryRequirements(butter->device, buffer, &mem_reqs);

  i32 memory_type = butter_find_memory_type(
      butter->physical_device, mem_reqs.memoryTypeBits,
      host_visible ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                   : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memory_type == -1 && !host_visible)
    memory_type = butter_find_memory_type(
        butter->physical_device, mem_reqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (memory_type == -1) {
    butter_log_fatal("Could not find suitable memory type");
    vkDestroyBuffer(butter->device, buffer, null);
    return (butter_buffer_t){0};
  }

  vk_memory_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = memory_type;

  vk_device_memory_t memory;
  if ((res = vkAllocateMemory(butter->device, &alloc_info, null, &memory)) !=
      VK_SUCCESS) {
    butter_log_fatal("Could not allocate buffer memory: %d", res);
    vkDestroyBuffer(butter->device, buffer, null);
    return (butter_buffer_t){0};
  }

  void *mapped;
  if ((res = vkMapMemory(butter->device, memory, 0, mem_reqs.size, 0,
                         &mapped)) != VK_SUCCESS) {
    butter_log_fatal("Could not map buffer memory: %d", res);
    vkDestroyBuffer(butter->device, buffer, null);
    vkFreeMemory(butter->device, memory, null);
    return (butter_buffer_t){0};
  }

  if ((res = vkBindBufferMemory(butter->device, buffer, memory, 0)) !=
      VK_SUCCESS) {
    butter_log_fatal("Could not bind buffer memory: %d", res);
    vkUnmapMemory(butter->device, memory);
    vkFreeMemory(butter->device, memory, null);
    vkDestroyBuffer(butter->device, buffer, null);
    return (butter_buffer_t){0};
  }

  butter_buffer_t butter_buffer = {0};
  butter_buffer.handle = buffer;
  butter_buffer.memory = memory;
  butter_buffer.size = size;
  butter_buffer.mapped = mapped;

  return butter_buffer;
}

void butter_destroy_buffer(butter_t *butter, butter_buffer_t *buffer) {
  if (!buffer || buffer->handle == VK_NULL_HANDLE)
    return;

  if (buffer->mapped) {
    vkUnmapMemory(butter->device, buffer->memory);
    buffer->mapped = null;
  }
  if (buffer->memory) {
    vkFreeMemory(butter->device, buffer->memory, NULL);
    buffer->memory = VK_NULL_HANDLE;
  }
  if (buffer->handle) {
    vkDestroyBuffer(butter->device, buffer->handle, NULL);
    buffer->handle = VK_NULL_HANDLE;
  }
}

void butter_buffer_upload(butter_t *butter, const butter_buffer_t *buffer,
                          const void *data, u64 size, u64 offset) {
  if (!butter || !buffer || !data || size == 0 ||
      buffer->handle == VK_NULL_HANDLE) {
    butter_log_error("Invalid arguments");
    return;
  }

  if (buffer->mapped) {
    memcpy((u8 *)buffer->mapped + offset, data, size);
    return;
  }

  butter_buffer_t staging = butter_create_buffer(
      butter, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
  if (staging.handle == VK_NULL_HANDLE) {
    butter_log_error("Could not create staging buffer");
    return;
  }

  memcpy(staging.mapped, data, size);

  vk_result_t res;

  vk_command_buffer_t cmd;
  vk_command_buffer_allocate_info_t cmd_alloc = {0};
  cmd_alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmd_alloc.commandPool = butter->upload_pool_sync;
  cmd_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd_alloc.commandBufferCount = 1;

  if ((res = vkAllocateCommandBuffers(butter->device, &cmd_alloc, &cmd)) !=
      VK_SUCCESS) {
    butter_log_error("Could not allocate upload command buffer: %d", res);
    butter_destroy_buffer(butter, &staging);
    return;
  }

  vk_command_buffer_begin_info_t begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if ((res = vkBeginCommandBuffer(cmd, &begin_info)) != VK_SUCCESS) {
    butter_log_error("Could not begin upload command buffer: %d", res);
    vkFreeCommandBuffers(butter->device, butter->upload_pool_sync, 1, &cmd);
    butter_destroy_buffer(butter, &staging);
    return;
  }

  vk_buffer_copy_t copy_region = {0};
  copy_region.srcOffset = 0;
  copy_region.dstOffset = offset;
  copy_region.size = size;

  vkCmdCopyBuffer(cmd, staging.handle, buffer->handle, 1, &copy_region);
  if ((res = vkEndCommandBuffer(cmd)) != VK_SUCCESS) {
    butter_log_error("Could not end upload command buffer: %d", res);
    vkFreeCommandBuffers(butter->device, butter->upload_pool_sync, 1, &cmd);
    butter_destroy_buffer(butter, &staging);
    return;
  }

  vk_fence_t fence;
  vk_fence_create_info_t fence_info = {0};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  if ((res = vkCreateFence(butter->device, &fence_info, null, &fence)) !=
      VK_SUCCESS) {
    butter_log_error("Could not create upload fence: %d", res);
    vkFreeCommandBuffers(butter->device, butter->upload_pool_sync, 1, &cmd);
    butter_destroy_buffer(butter, &staging);
    return;
  }

  vk_submit_info_t submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd;

  vkQueueSubmit(butter->queue, 1, &submit_info, fence);
  vkWaitForFences(butter->device, 1, &fence, true, UINT64_MAX);

  vkDestroyFence(butter->device, fence, null);
  vkFreeCommandBuffers(butter->device, butter->upload_pool_sync, 1, &cmd);
  butter_destroy_buffer(butter, &staging);
}

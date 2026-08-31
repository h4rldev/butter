/***********************************/

#include <htils/basictypes.h>

#include <butter/log.h>
#include <butter/types.h>

/***********************************/

vk_descriptor_pool_t
butter_create_descriptor_pool(butter_t *butter, u32 max_sets,
                              const vk_descriptor_pool_size_t *pool_sizes,
                              u32 pool_size_count) {
  vk_descriptor_pool_create_info_t pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.maxSets = max_sets;
  pool_info.poolSizeCount = pool_size_count;
  pool_info.pPoolSizes = pool_sizes;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

  vk_descriptor_pool_t pool;
  if (vkCreateDescriptorPool(butter->device, &pool_info, null, &pool) !=
      VK_SUCCESS) {
    butter_log_error("Could not create descriptor pool");
    return VK_NULL_HANDLE;
  }

  return pool;
}

void butter_destroy_descriptor_pool(butter_t *butter,
                                    vk_descriptor_pool_t pool) {
  if (pool)
    vkDestroyDescriptorPool(butter->device, pool, null);
}

vk_descriptor_set_layout_t butter_create_descriptor_set_layout(
    butter_t *butter, vk_descriptor_set_layout_binding_t *bindings,
    u32 binding_count, vk_descriptor_set_layout_create_flags_t flags) {

  vk_descriptor_set_layout_create_info_t layout_info = {0};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.flags = flags;
  layout_info.bindingCount = binding_count;
  layout_info.pBindings = bindings;

  vk_descriptor_set_layout_t layout;
  vk_result_t res;
  if ((res = vkCreateDescriptorSetLayout(butter->device, &layout_info, null,
                                         &layout)) != VK_SUCCESS) {
    butter_log_error("Could not create descriptor set layout: %d", res);
    return VK_NULL_HANDLE;
  }

  return layout;
}

butter_descriptor_set_t
butter_allocate_descriptor_set(butter_t *butter, vk_descriptor_pool_t pool,
                               vk_descriptor_set_layout_t layout) {
  vk_result_t res;

  butter_descriptor_set_t set = {0};
  set.layout = layout;

  vk_descriptor_set_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &layout;

  if ((res = vkAllocateDescriptorSets(butter->device, &alloc_info, &set.set)) !=
      VK_SUCCESS) {
    butter_log_error("Could not allocate descriptor set: %d", res);
    return (butter_descriptor_set_t){0};
  }

  return set;
}

void butter_update_descriptor_buffer(butter_t *butter,
                                     butter_descriptor_set_t *set, u32 binding,
                                     vk_buffer_t buffer, u64 offset, u64 size) {
  vk_descriptor_buffer_info_t buffer_info = {0};
  buffer_info.buffer = buffer;
  buffer_info.offset = offset;
  buffer_info.range = size;

  vk_write_descriptor_set_t descriptor_write = {0};
  descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_write.dstSet = set->set;
  descriptor_write.dstBinding = binding;
  descriptor_write.dstArrayElement = 0;
  descriptor_write.descriptorCount = 1;
  descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptor_write.pBufferInfo = &buffer_info;

  vkUpdateDescriptorSets(butter->device, 1, &descriptor_write, 0, null);
}

void butter_update_descriptor_image(butter_t *butter,
                                    butter_descriptor_set_t *set, u32 binding,
                                    vk_image_view_t image_view,
                                    vk_sampler_t sampler) {
  vk_descriptor_image_info_t image_info = {0};
  image_info.imageView = image_view;
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.sampler = sampler;

  vk_write_descriptor_set_t descriptor_write = {0};
  descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptor_write.dstSet = set->set;
  descriptor_write.dstBinding = binding;
  descriptor_write.dstArrayElement = 0;
  descriptor_write.descriptorCount = 1;
  descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptor_write.pImageInfo = &image_info;

  vkUpdateDescriptorSets(butter->device, 1, &descriptor_write, 0, null);
}

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/internal/memory.h>
#include <butter/internal/texture.h>
#include <butter/internal/types.h>

#include <butter/graphics.h>
#include <butter/log.h>
#include <butter/texture.h>
#include <butter/types.h>

/***********************************/

#define BUTTER_TEXTURE_POOL_CHUNK (64)

/**
 * @brief Create and record a new texture descriptor pool.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The new pool, or VK_NULL_HANDLE on failure.
 */
static vk_descriptor_pool_t butter_texture_pool_add(butter_context_t *context) {
  vk_descriptor_pool_size_t pool_size = {0};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = BUTTER_TEXTURE_POOL_CHUNK;

  vk_descriptor_pool_t pool = butter_create_descriptor_pool(
      context, BUTTER_TEXTURE_POOL_CHUNK, &pool_size, 1);
  if (pool == VK_NULL_HANDLE)
    return VK_NULL_HANDLE;

  vk_descriptor_pool_t *pools =
      arena_alloc_zeroed(context->arena, vk_descriptor_pool_t,
                         context->texture_descriptor_pool_count + 1);
  if (!pools) {
    butter_destroy_descriptor_pool(context, pool);
    return VK_NULL_HANDLE;
  }

  for (u32 i = 0; i < context->texture_descriptor_pool_count; i++)
    pools[i] = context->texture_descriptor_pools[i];

  pools[context->texture_descriptor_pool_count] = pool;

  context->texture_descriptor_pools = pools;
  context->texture_descriptor_pool_count++;

  return pool;
}

//
//
//

/**
 * @brief Get the most recently created texture descriptor pool.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The newest pool, or VK_NULL_HANDLE if none exist.
 */
static vk_descriptor_pool_t
butter_texture_pool_current(butter_context_t *context) {
  if (context->texture_descriptor_pool_count == 0)
    return VK_NULL_HANDLE;

  return context
      ->texture_descriptor_pools[context->texture_descriptor_pool_count - 1];
}

//
//
//

b32 butter_texture_create_descriptor_set(butter_context_t *context,
                                         struct butter_texture *texture) {
  if (context->available_vulkan_features & BUTTER_FEATURE_PUSH_DESCRIPTORS)
    return true;

  b32 ok = false;
  mtx_lock(&context->texture_descriptor_mutex);

  vk_descriptor_pool_t pool = butter_texture_pool_current(context);

  for (;;) {
    if (pool == VK_NULL_HANDLE)
      pool = butter_texture_pool_add(context);

    if (pool == VK_NULL_HANDLE) {
      butter_log_error("Could not create texture descriptor pool");
      break;
    }

    vk_descriptor_set_allocate_info_t alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &context->texture_descriptor_set_layout;

    vk_descriptor_set_t set = VK_NULL_HANDLE;
    vk_result_t res;

    if ((res = vkAllocateDescriptorSets(context->device, &alloc_info, &set)) ==
        VK_SUCCESS) {
      butter_descriptor_set_t descriptor_set = {
          .set = set, .layout = context->texture_descriptor_set_layout};
      butter_update_descriptor_image(context, &descriptor_set, 0, texture->view,
                                     texture->sampler);

      texture->descriptor_set = descriptor_set;
      texture->descriptor_pool = pool;
      ok = true;
      break;
    }

    if (res != VK_ERROR_OUT_OF_POOL_MEMORY && res != VK_ERROR_FRAGMENTED_POOL) {
      butter_log_error("Could not allocate descriptor set: %d", res);
      break;
    }

    pool = VK_NULL_HANDLE;
  }

  mtx_unlock(&context->texture_descriptor_mutex);
  return ok;
}

b32 butter_texture_create_image(butter_context_t *context,
                                struct butter_texture *texture,
                                vk_image_usage_flags_t usage) {
  vk_image_create_info_t image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = texture->format;
  image_info.extent = (vk_extent3d_t){texture->width, texture->height, 1};
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.usage = usage;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  vk_result_t res;
  if ((res = vkCreateImage(context->device, &image_info, null,
                           &texture->image)) != VK_SUCCESS) {
    butter_log_error("Could not create image: %d", res);
    return false;
  }

  vk_memory_requirements_t mem_reqs;
  vkGetImageMemoryRequirements(context->device, texture->image, &mem_reqs);
  i32 mem_type =
      butter_find_memory_type(context->physical_device, mem_reqs.memoryTypeBits,
                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (mem_type == -1) {
    butter_log_fatal("No device-local memory type for texture");
    return false;
  }

  vk_memory_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = mem_type;

  if ((res = vkAllocateMemory(context->device, &alloc_info, null,
                              &texture->memory)) != VK_SUCCESS) {
    butter_log_error("Could not allocate memory for texture: %d", res);
    return false;
  }

  if ((res = vkBindImageMemory(context->device, texture->image, texture->memory,
                               0)) != VK_SUCCESS) {
    butter_log_error("Could not bind image memory: %d", res);
    return false;
  }

  return true;
}

//
//
//

b32 butter_texture_create_view(butter_context_t *context,
                               struct butter_texture *texture) {
  vk_image_view_create_info_t image_view_info = {0};
  image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_info.image = texture->image;
  image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  image_view_info.format = texture->format;
  image_view_info.subresourceRange = (vk_image_subresource_range_t){0};
  image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  image_view_info.subresourceRange.levelCount = 1;
  image_view_info.subresourceRange.layerCount = 1;

  vk_result_t res;
  if ((res = vkCreateImageView(context->device, &image_view_info, null,
                               &texture->view)) != VK_SUCCESS) {
    butter_log_error("Could not create image view: %d", res);
    return false;
  }

  return true;
}

//
//
//

b32 butter_texture_init_target(butter_context_t *context,
                               struct butter_texture *texture, u32 width,
                               u32 height, vk_format_t format,
                               vk_sampler_t sampler,
                               vk_image_usage_flags_t usage) {
  if (!context || !texture || width == 0 || height == 0 ||
      format == VK_FORMAT_UNDEFINED || sampler == VK_NULL_HANDLE) {
    butter_log_error("Invalid arguments for texture target");
    return false;
  }

  texture->width = width;
  texture->height = height;
  texture->format = format;
  texture->sampler = sampler;
  atomic_store(&texture->is_upload, false);
  atomic_store(&texture->upload_ready, true);
  atomic_store(&texture->upload_failed, false);
  atomic_store(&texture->upload_cancelled, false);

  if (!butter_texture_create_image(context, texture, usage))
    return false;

  if (!butter_texture_create_view(context, texture)) {
    butter_destroy_texture(context, texture);
    return false;
  }

  if (!butter_texture_create_descriptor_set(context, texture)) {
    butter_destroy_texture(context, texture);
    return false;
  }

  return true;
}

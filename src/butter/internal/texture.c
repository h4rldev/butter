/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/graphics.h>
#include <butter/internal/texture.h>
#include <butter/internal/types.h>
#include <butter/log.h>
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

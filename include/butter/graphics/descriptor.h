#ifndef BUTTER_GRAPHICS_DESCRIPTOR_H
#define BUTTER_GRAPHICS_DESCRIPTOR_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Create a new descriptor pool.
 *
 * @details Uses vkCreateDescriptorPool to create a new descriptor pool with the
 * provided amount of max sets, pool sizes and amount of pool sizes.
 *
 * @param butter The butter context.
 * @param max_sets The max amount of sets the pool can hold.
 * @param pool_sizes The pool sizes.
 * @param pool_size_count The amount of pool sizes.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c max_sets must be greater than 0.
 * - @c pool_sizes must be a valid pointer to an array of pool sizes.
 * - @c pool_size_count must be greater than 0 and match the size of the pool
 * sizes array ptr.
 *
 * @return The new descriptor pool.
 */
vk_descriptor_pool_t
butter_create_descriptor_pool(butter_t *butter, u32 max_sets,
                              const vk_descriptor_pool_size_t *pool_sizes,
                              u32 pool_size_count);

//
//
//

/**
 * @brief Destroy a descriptor pool if it is not null.
 *
 * @details Uses vkDestroyDescriptorPool to destroy the descriptor pool.
 *
 * @param butter The butter context.
 * @param pool The descriptor pool to destroy.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c pool must be a valid descriptor pool.
 */
void butter_destroy_descriptor_pool(butter_t *butter,
                                    vk_descriptor_pool_t pool);

//
//
//

/**
 * @brief Create a new descriptor set layout.
 *
 * @details Uses vkCreateDescriptorSetLayout to create a new descriptor set
 * layout with the bindings provided.
 *
 * @param butter The butter context.
 * @param bindings The bindings for the descriptor set layout.
 * @param binding_count The amount of bindings.
 * @param flags The flags for the descriptor set layout, see
 * VkDescriptorSetLayoutCreateFlags for more info.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c bindings must be a valid pointer to an array of bindings.
 * - @c binding_count must be greater than 0 and match the size of the bindings
 * array ptr.
 * - @c flags must be a valid flags bitmask.
 *
 * @return The new descriptor set layout.
 */
vk_descriptor_set_layout_t butter_create_descriptor_set_layout(
    butter_t *butter, vk_descriptor_set_layout_binding_t *bindings,
    u32 binding_count, vk_descriptor_set_layout_create_flags_t flags);

//
//
//

/**
 * @brief Allocate a new descriptor set.
 *
 * @details Uses vkAllocateDescriptorSets to allocate a new descriptor set with
 * the layout and pool provided, while keeping both the set and the layout
 * alive.
 *
 * @param butter The butter context.
 * @param pool The descriptor pool to allocate from.
 * @param layout The descriptor set layout to allocate from.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c pool must be a valid descriptor pool.
 * - @c layout must be a valid descriptor set layout.
 *
 * @return The new descriptor set holding both the set and the set layout.
 */
butter_descriptor_set_t
butter_allocate_descriptor_set(butter_t *butter, vk_descriptor_pool_t pool,
                               vk_descriptor_set_layout_t layout);

//
//
//

/**
 * @brief Update a descriptor set with a buffer.
 *
 * @todo Make multi-set.
 *
 * @param butter The butter context.
 * @param set The descriptor set to update.
 * @param binding The binding to update.
 * @param buffer The buffer to use.
 * @param offset The offset in the buffer to use.
 * @param size The size of the buffer to use.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c set must be a valid descriptor set.
 * - @c binding must be a valid binding.
 * - @c buffer must be a valid buffer.
 * - @c offset must be a valid offset.
 * - @c size must be a valid size.
 */
void butter_update_descriptor_buffer(butter_t *butter,
                                     butter_descriptor_set_t *set, u32 binding,
                                     vk_buffer_t buffer, u64 offset, u64 size);

//
//
//

/**
 * @brief Update a descriptor set with an image view and a sampler.
 *
 * @todo Make multi-set.
 *
 * @param butter The butter context.
 * @param set The descriptor set to update.
 * @param binding The binding to update.
 * @param image_view The image view to use.
 * @param sampler The sampler to use.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c set must be a valid descriptor set.
 * - @c binding must be a valid binding.
 * - @c image_view must be a valid image view.
 * - @c sampler must be a valid sampler.
 */
void butter_update_descriptor_image(butter_t *butter,
                                    butter_descriptor_set_t *set, u32 binding,
                                    vk_image_view_t image_view,
                                    vk_sampler_t sampler);

#endif // !BUTTER_GRAPHICS_DESCRIPTOR_H

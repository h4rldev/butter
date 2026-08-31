#ifndef BUTTER_INTERNAL_GET_H
#define BUTTER_INTERNAL_GET_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Get the required instance extensions.
 * @details Gets the required instance extensions for the given backend and sets
 * the count to the number of extensions.
 *
 * @param backend The backend.
 * @param count The count of extensions.
 *
 * @pre
 * - @c backend must be a valid backend.
 * - @c count must not be null.
 *
 * @return The required instance extensions.
 */
const cstr *const *
butter_get_required_instance_extensions(butter_backend_t backend, u32 *count);

//
//
//

/**
 * @brief Get the device from the context.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The device.
 */
vk_device_t butter_get_device(butter_context_t *context);

//
//
//

/**
 * @brief Get the frame index from the context.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The frame index.
 */
u32 butter_get_frame_index(butter_context_t *context);

//
//
//

/**
 * @brief Get the image count from the context.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The image count.
 */
u32 butter_get_image_count(butter_context_t *context);

//
//
//

/**
 * @brief Get the format from the context.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The format.
 */
vk_format_t butter_get_format(butter_context_t *context);

//
//
//

/**
 * @brief Get the extent from the context.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The extent.
 */
vk_extent2d_t butter_get_extent(butter_context_t *context);

//
//
//

/**
 * @brief Get the default render pass from the context.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The default render pass.
 */
vk_render_pass_t butter_get_default_render_pass(butter_context_t *context);

//
//
//

/**
 * @brief Get the framebuffer from the context for the given image index.
 *
 * @param context The butter context.
 * @param image_index The image index.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c image_index must be a valid image index.
 *
 * @return The framebuffer.
 */
vk_framebuffer_t butter_get_framebuffer(butter_context_t *context,
                                        u32 image_index);

//
//
//

/**
 * @brief Get the queue from the context.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The queue.
 */
vk_queue_t butter_get_queue(butter_context_t *context);

//
//
//

/**
 * @brief Get the queue family from the context.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The queue family.
 */
u32 butter_get_queue_family(butter_context_t *context);

//
//
//

/**
 * @brief Get the shared command pool from the context.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The shared command pool.
 */
vk_command_pool_t butter_get_cmd_pool(butter_context_t *context);

//
//
//

/**
 * @brief Get the command buffer from the context for the given image index.
 *
 * @param context The butter context.
 * @param image_index The image index.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c image_index must be a valid image index.
 *
 * @return The command buffer.
 */
vk_command_buffer_t butter_get_cmd(butter_context_t *context, u32 image_index);

#endif // !BUTTER_INTERNAL_GET_H

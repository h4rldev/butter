#ifndef BUTTER_GRAPHICS_BUFFER_H
#define BUTTER_GRAPHICS_BUFFER_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Create a new buffer.
 *
 * @details Uses vkCreateBuffer to create with the given size, usage flags and
 * whether the buffer should be host visible.
 *
 * @param butter The butter context.
 * @param size The size of the buffer.
 * @param usage The usage flags for the buffer, check VkBufferUsageFlags for
 * more info.
 * @param host_visible Whether the buffer should be host visible.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c size must be greater than 0.
 * - @c usage must be a valid usage flags bitmask.
 *
 * @return The new buffer.
 */
butter_buffer_t butter_create_buffer(butter_t *butter, u64 size,
                                     vk_buffer_usage_flags_t usage,
                                     b32 host_visible);

//
//
//

/**
 * @brief Destroy a buffer.
 *
 * @details Uses vkDestroyBuffer to destroy the butter_buffer.
 *
 * @param butter The butter context.
 * @param buffer The buffer to destroy.
 *
 * @pre
 * - @c buffer must be a valid buffer
 * - @c butter must be a valid butter context.
 */
void butter_destroy_buffer(butter_t *butter, butter_buffer_t *buffer);

//
//
//

/**
 * @brief Upload data to a buffer.
 *
 * @details If the buffer is host-visible, the data is copied directly into the
 * mapped memory. Otherwise a staging buffer is used to copy the data to the
 * buffer on the device (synchronously).
 *
 * @param butter The butter context.
 * @param buffer The buffer to upload to.
 * @param data The data to upload.
 * @param size The amount of data to upload.
 * @param offset The offset in the buffer to upload to.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c buffer must be a valid buffer, created with
 * @c VK_BUFFER_USAGE_TRANSFER_DST_BIT if it is not host-visible.
 * - @c data must be a valid pointer to @c size bytes.
 * - @c size must be greater than 0.
 */
void butter_buffer_upload(butter_t *butter, const butter_buffer_t *buffer,
                          const void *data, u64 size, u64 offset);

#endif // !BUTTER_GRAPHICS_BUFFER_H

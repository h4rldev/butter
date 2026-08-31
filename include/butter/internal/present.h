#ifndef BUTTER_INTERNAL_PRESENT_H
#define BUTTER_INTERNAL_PRESENT_H

/***********************************/

#include <butter/types.h>
#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

/***********************************/

/**
 * @brief Acquire the next image to present.
 * @details Acquires the next image to present, if timeline semaphores aren't
 * available, it waits for the fence, if timeline semaphores are available, it
 * waits for the timeline semaphore.
 *
 * @param context The butter context.
 * @param image_index The image index to acquire.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c image_index must not be null.
 *
 * @return The result of the acquire.
 */
vk_result_t butter_acquire_next_image(butter_context_t *context,
                                      u32 *image_index);

//
//
//

/**
 * @brief Submit the command buffer and present the image.
 * @details Submits the command buffer, if timeline semaphores aren't available,
 * it waits for the fence, if timeline semaphores are available, it waits for
 * the timeline and sends a signal to the rendering finished semaphore.
 *
 * @param context The butter context.
 * @param cmd The command buffer to submit.
 * @param image_index The image index to present.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c cmd must be a valid command buffer.
 * - @c image_index must be a valid image index.
 *
 * @return The result of the submit.
 */
vk_result_t butter_submit_and_present(butter_context_t *context,
                                      vk_command_buffer_t cmd, u32 image_index);

#endif // !BUTTER_INTERNAL_PRESENT_H

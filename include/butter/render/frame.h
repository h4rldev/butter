#ifndef BUTTER_RENDER_FRAME_H
#define BUTTER_RENDER_FRAME_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Begin a frame manually.
 * @details Acquires the next swapchain image, resets and begins the command
 * buffer, and begins the render pass, returning a new frame. Use this for
 * manual frame control; alternatively @ref butter_start_render_thread drives
 * frames automatically.
 *
 * @param arena The arena to allocate the frame from.
 * @param butter The butter context.
 *
 * @pre
 * - @c arena must be a valid arena.
 * - @c butter must be a valid butter context.
 *
 * @return The new frame, or null if the frame could not be acquired.
 */
butter_frame_t *butter_begin_frame(arena_t *arena, butter_t *butter);

//
//
//

/**
 * @brief End a frame.
 * @details Ends a frame, submits the command buffer, and presents the image.
 *
 * @param arena The arena to allocate the frame from.
 * @param butter The butter context.
 * @param frame The frame to end.
 *
 * @pre
 * - @c arena must be a valid arena.
 * - @c butter must be a valid butter context.
 * - @c frame must be a valid frame.
 *
 * @return The result of the submit.
 */
vk_result_t butter_end_frame(arena_t *arena, butter_t *butter,
                             butter_frame_t *frame);

//
//
//

/**
 * @brief Resize the swapchain.
 * @details Synchronously resizes the swapchain to the given dimensions: waits
 * for the device to idle, recreates the swapchain, and reallocates the
 * command buffers. Use @ref butter_set_pending_resize for an async/deferred
 * resize that the render thread applies.
 *
 * @param butter The butter context.
 * @param width The new width.
 * @param height The new height.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c width and @c height must be greater than 0.
 */
void butter_resize(butter_t *butter, u32 width, u32 height);

#endif // !BUTTER_RENDER_FRAME_H

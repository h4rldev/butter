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
 * @brief Snapshot the current frame into a target's texture.
 * @details Copies the current pass's colour attachment (a region of it, or the
 * whole extent when @c width / @c height are 0) into @c dst's texture and
 * leaves it sampleable, so an effect can read the frame so far. The pass is
 * broken and resumed internally, so drawing continues in order.
 * @details The destination is a live target the caller owns, created once and
 * rebuilt by butter on resize and anti-aliasing changes; no per-frame GPU
 * allocation happens, so it is safe to call every frame. Use a dedicated
 * target, not the target of the open pass, and keep it alive until the frame's
 * GPU work completes.
 *
 * @param butter The butter context.
 * @param dst The target to copy the frame into.
 * @param x The region x offset, in pixels.
 * @param y The region y offset, in pixels.
 * @param width The region width; 0 means to the right edge.
 * @param height The region height; 0 means to the bottom edge.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c dst must be a valid target, not the target of the open pass.
 * - @ref butter_begin_frame must have been called and its pass still open.
 * - The region must lie inside the current extent and fit @c dst.
 *
 * @return @c dst's texture, or null on failure.
 */
butter_texture_t *butter_snapshot(butter_t *butter, butter_target_t *dst, u32 x,
                                  u32 y, u32 width, u32 height);

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

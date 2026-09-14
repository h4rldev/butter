#ifndef BUTTER_RENDER_THREAD_H
#define BUTTER_RENDER_THREAD_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/types.h>

/***********************************/

/**
 * @brief Start the render thread.
 * @details Starts the render thread, if the render thread is already running,
 * it's a no-op.
 *
 * @param butter The butter context.
 * @param per_frame_arena The per frame arena.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c per_frame_arena must be a valid arena.
 */
void butter_start_render_thread(butter_t *butter, arena_t *per_frame_arena);

//
//
//

/**
 * @brief Stop the render thread.
 * @details Stops the render thread, if the render thread is already stopped,
 * it's a no-op.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_stop_render_thread(butter_t *butter);

//
//
//

/**
 * @brief Request a frame.
 * @details Requests a frame to be rendered. If the render thread isn't running,
 * it's a no-op.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_request_frame(butter_t *butter);

//
//
//

/**
 * @brief Wait for a frame.
 * @details Waits for a frame to be rendered. If the render thread isn't
 * running, it's a no-op.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_wait_for_frame(butter_t *butter);

//
//
//

/**
 * @brief Check if the render thread is running.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 *
 * @return true if the render thread is running, false otherwise.
 */
b32 butter_is_render_thread_running(const butter_t *butter);

/**
 * @brief Check if the last requested frame has completed.
 * @details Non-blocking. Use this to poll completion while keeping the
 * caller's event loop alive (e.g. dispatching the window system) instead of
 * blocking in @ref butter_wait_for_frame.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 *
 * @return true if the last requested frame has completed.
 */
b32 butter_frame_completed(const butter_t *butter);

#endif // !BUTTER_RENDER_THREAD_H

#ifndef BUTTER_RENDER_H
#define BUTTER_RENDER_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/types.h>

/***********************************/

/**
 * @brief Get the default init config.
 * @details Gets the default init config, with the following settings:
 * - app_name: butter
 * - use_validation_layers: false
 * - latency_cap: 0
 * - width: 0
 * - height: 0
 * - dynamic_vbo_size: 0
 * - dynamic_ibo_size: 0
 * - pipeline_cache_path: null
 * - enable_depth: false
 *
 * @return The default init config.
 */
butter_init_config_t butter_init_config_default(void);

//
//
//

/**
 * @brief Initialize the butter context.
 * @details Initializes the butter context with the given arena, surface info,
 * including instance, device, surface, command pool, and command vector. Serves
 * as an abstraction and extension for @ref butter_create, for library users.
 *
 * @param arena The arena to allocate the context from.
 * @param surface_info The surface info.
 * @param config The configuration, see @ref butter_init_config_t for more info.
 *
 * @pre
 * - @c arena must be a valid arena.
 * - @c surface_info must be a valid surface info.
 * - @c config must be a valid init configuration.
 *
 * @return The new butter context.
 */
butter_t *butter_init(arena_t *arena, butter_surface_info_t *surface_info,
                      const butter_init_config_t *config);

//
//
//

/**
 * @brief Destroy the butter context.
 * @details Destroys the command pool, and calls @ref butter_destroy on the
 * context.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_end(butter_t *butter);

//
//
//

/**
 * @brief Set the clear color.
 * @details Sets the clear color for the next frame.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_set_clear_color(butter_t *butter, f32 r, f32 g, f32 b, f32 a);

//
//
//

/**
 * @brief Begin a frame.
 * @details Begins a frame, acquires the next image, begins the render pass and
 * allocates a new frame, not intended for multi-thread use, if you want
 * multithreaded rendering, use @ref butter_start_render_thread instead.
 *
 * @param arena The arena to allocate the frame from.
 * @param butter The butter context.
 *
 * @pre
 * - @c arena must be a valid arena.
 * - @c butter must be a valid butter context.
 *
 * @return The new frame.
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
 * @brief Schedule a resize.
 * @details Schedules a resize, if the resize is pending, it will be executed,
 * refreshing the render.
 *
 * @param butter The butter context.
 * @param width The width.
 * @param height The height.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c width must be a valid width.
 * - @c height must be a valid height.
 */
void butter_resize(butter_t *butter, u32 width, u32 height);

//
//
//

/**
 * @brief Set the draw callback.
 * @details Sets the draw callback for the butter context, for the multi-thread
 * render, not for single-threaded use.
 *
 * @param butter The butter context.
 * @param cb The draw callback.
 * @param userdata The userdata for the callback.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c cb must be a valid draw callback.
 * - @c userdata must be a valid userdata.
 */
void butter_set_draw_callback(butter_t *butter, butter_draw_callback_t cb,
                              void *userdata);

//
//
//

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

//
//
//

/**
 * @brief Toggle VSync.
 * @details Sets VSync on or off.
 *
 * @param butter The butter context.
 * @param vsync The boolean value to set VSync to.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_set_vsync(butter_t *butter, b32 vsync);

//
//
//

/**
 * @brief Set the target refresh rate.
 *
 * @param butter The butter context.
 * @param rate The target refresh rate.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c rate must be a valid refresh rate in Hz.
 */
void butter_set_target_refresh_rate(butter_t *butter, f32 rate);

//
//
//

/**
 * @brief Get the target refresh rate.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 *
 * @return The target refresh rate stored in the butter context.
 */
f32 butter_get_target_refresh_rate(const butter_t *butter);

//
//
//

/**
 * @brief Set the pending resize.
 * @details Sets the resize to pending, with the provided width and height.
 *
 * @param butter The butter context.
 * @param width The width.
 * @param height The height.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c width must be a valid width.
 * - @c height must be a valid height.
 */
void butter_set_pending_resize(butter_t *butter, u32 width, u32 height);

#endif // !BUTTER_RENDER_H

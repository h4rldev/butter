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
 * and configuration. Creates the Vulkan instance, physical device, logical
 * device, surface, swapchain, command pool, dynamic vertex/index buffers,
 * synchronization primitives, and texture registry — the full context in one
 * call. Serves as the user-facing entry point over the internal
 * @ref butter_create.
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
 * @return The new butter context, or null on failure.
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
 * @details Sets the clear color used when clearing the swapchain images at the
 * start of each frame. Colors are in the range 0.0 - 1.0.
 *
 * @param butter The butter context.
 * @param r The red component.
 * @param g The green component.
 * @param b The blue component.
 * @param a The alpha component.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_set_clear_color(butter_t *butter, f32 r, f32 g, f32 b, f32 a);

//
//
//

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

//
//
//

/**
 * @brief Set the draw callback for the render thread.
 * @details Sets the callback invoked by the render thread for each frame.
 * Only used when rendering via @ref butter_start_render_thread; manual
 * frame control (@ref butter_begin_frame / @ref butter_end_frame) does not
 * invoke it.
 *
 * @param butter The butter context.
 * @param cb The draw callback.
 * @param userdata The userdata for the callback.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c cb must be a valid draw callback.
 * - @c userdata must be a valid pointer or null.
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
 * @brief Set a pending resize.
 * @details Records a resize request that the render thread applies at the
 * start of the next frame. Use this for deferred resizes; see
 * @ref butter_resize for a synchronous resize.
 *
 * @param butter The butter context.
 * @param width The new width.
 * @param height The new height.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c width and @c height must be greater than 0.
 */
void butter_set_pending_resize(butter_t *butter, u32 width, u32 height);

#endif // !BUTTER_RENDER_H

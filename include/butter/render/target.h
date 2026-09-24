#ifndef BUTTER_RENDER_TARGET_H
#define BUTTER_RENDER_TARGET_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Create an offscreen render target.
 * @details A target is an offscreen colour attachment (plus a depth attachment
 * when the context enables depth) that can also be sampled as a texture, so an
 * effect can read what was rendered into it. When anti-aliasing is enabled the
 * target renders multisampled and resolves into its sampleable texture.
 * @details A target is live: when the frame is resized or anti-aliasing
 * changes, butter rebuilds its GPU resources in place and the handle stays
 * valid. A dimension given as 0 follows the frame's corresponding dimension; a
 * non-zero dimension stays fixed. The swapchain is the implicit target 0: pass
 * @c null to @ref butter_pass_begin to render to the frame.
 *
 * @param butter The butter context.
 * @param arena The arena to allocate the target from.
 * @param desc The target description.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c arena must be a valid arena.
 * - @c desc must be a valid target description.
 *
 * @return The new target, or null on failure.
 */
butter_target_t *butter_target_create(butter_t *butter, arena_t *arena,
                                      const butter_target_desc_t *desc);

//
//
//

/**
 * @brief Destroy a render target and its GPU resources.
 * @details Removes the target from the context; destroy it before clearing the
 * arena it was allocated from. Create and destroy targets on the thread that
 * renders.
 *
 * @param butter The butter context.
 * @param target The target to destroy.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c target must be a valid target.
 */
void butter_target_destroy(butter_t *butter, butter_target_t *target);

//
//
//

/**
 * @brief The target's colour attachment as a sampleable texture.
 * @details The texture is owned by the target; do not destroy it, and do not
 * use it after destroying the target.
 *
 * @param target The target.
 *
 * @return The target's texture, or null if @c target is null.
 */
butter_texture_t *butter_target_texture(butter_target_t *target);

//
//
//

/**
 * @brief Begin a render pass on a target.
 * @details Pass @c null to render to the frame's swapchain target. A pass must
 * be ended with @ref butter_pass_end before another begins. To break and resume
 * a pass (so an effect can read the frame so far) end it, then begin again with
 * @c color_load set to true.
 *
 * @param butter The butter context.
 * @param target The target to render to, or null for the swapchain.
 * @param color_load Load the colour attachment instead of clearing it. Ignored
 * for a @ref butter_target_t, whose pass always clears so the result can be
 * sampled.
 * @param depth_load Reserved; the depth attachment shares the colour choice.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @ref butter_begin_frame must have been called.
 * - No pass may already be open.
 */
void butter_pass_begin(butter_t *butter, butter_target_t *target,
                       b32 color_load, b32 depth_load);

//
//
//

/**
 * @brief End the current render pass.
 *
 * @param butter The butter context.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - A pass must be open.
 */
void butter_pass_end(butter_t *butter);

#endif // !BUTTER_RENDER_TARGET_H

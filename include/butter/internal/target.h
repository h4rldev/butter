#ifndef BUTTER_INTERNAL_TARGET_H
#define BUTTER_INTERNAL_TARGET_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>

/***********************************/

/**
 * @brief Build a target's GPU resources.
 * @details Creates the colour texture, the MSAA colour image (when
 * anti-aliasing is on), the depth image (when depth is enabled) and the
 * framebuffer, from the target's current @c width/@c height and the context's
 * format, sample count and depth setting.
 *
 * @param butter The butter context.
 * @param target The target to build.
 *
 * @pre
 * - @c butter must be a valid butter context with a target render pass.
 * - @c target->width and @c target->height must be non-zero.
 *
 * @return True on success, false otherwise.
 */
b32 butter_target_build(butter_context_t *butter, struct butter_target *target);

//
//
//

/**
 * @brief Destroy a target's GPU resources, leaving the struct reusable.
 *
 * @param butter The butter context.
 * @param target The target to tear down.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_target_teardown(butter_context_t *butter,
                            struct butter_target *target);

//
//
//

/**
 * @brief Rebuild a target for the current frame size and sample count.
 * @details No-op when the size and sample count are unchanged. Dimensions that
 * were requested as 0 follow the frame extent.
 *
 * @param butter The butter context.
 * @param target The target to rebuild.
 *
 * @return True on success, false otherwise.
 */
b32 butter_target_rebuild(butter_context_t *butter,
                          struct butter_target *target);

//
//
//

/**
 * @brief Rebuild every live target for the current frame state.
 *
 * @param context The butter context.
 *
 * @return True on success, false otherwise.
 */
b32 butter_rebuild_targets(butter_context_t *context);

//
//
//

/**
 * @brief Destroy every live target's GPU resources and clear the registry.
 *
 * @param context The butter context.
 */
void butter_destroy_targets(butter_context_t *context);

#endif // !BUTTER_INTERNAL_TARGET_H

#ifndef BUTTER_INTERNAL_STATS_H
#define BUTTER_INTERNAL_STATS_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>

/***********************************/

/**
 * @brief Create the timestamp query pool. No-op if timestamps unsupported.
 *
 * @param context The butter context.
 */
void butter_stats_init(butter_context_t *context);

//
//
//

/**
 * @brief Destroy the timestamp query pool.
 *
 * @param context The butter context.
 */
void butter_stats_destroy(butter_context_t *context);

//
//
//

/**
 * @brief Record a frame's start timestamp and read the previous frame's GPU
 * time for this slot.
 *
 * @param context The butter context.
 * @param cmd The active command buffer.
 * @param slot The in-flight frame slot.
 */
void butter_stats_begin_frame(butter_context_t *context,
                              vk_command_buffer_t cmd, u32 slot);

//
//
//

/**
 * @brief Write this frame's end timestamp.
 * @details Called from butter_end_frame BEFORE vkEndCommandBuffer, while the
 * command buffer is still recording.
 *
 * @param context The butter context.
 * @param cmd The active, recording command buffer.
 * @param slot The in-flight frame slot.
 */
void butter_stats_end_timestamp(butter_context_t *context,
                                vk_command_buffer_t cmd, u32 slot);

//
//
//

/**
 * @brief Store CPU frame time and refresh VRAM usage.
 * @details Called from butter_end_frame AFTER vkEndCommandBuffer.
 *
 * @param context The butter context.
 * @param cpu_frame_ms Render-thread CPU time spent on this frame.
 */
void butter_stats_record_cpu(butter_context_t *context, f32 cpu_frame_ms);

//
//
//

/**
 * @brief Accumulate a frame and recompute FPS over a fixed window, Averiging
 * avoids the per-frame jitter of vsync.
 *
 * @param context The butter context.
 * @param now_ns The current time.
 */
void butter_stats_tick_fps(butter_context_t *context, u64 now_ns);

#endif // !BUTTER_INTERNAL_STATS_H

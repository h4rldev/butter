/***********************************/

#include <stdatomic.h>

#include <htils/basictypes.h>

#include <butter/types.h>

/***********************************/

b32 butter_get_stats(butter_t *butter, butter_stats_t *out) {
  butter_context_t *context = (butter_context_t *)butter;
  if (!context || !out)
    return false;
  if (!atomic_load(&context->stats.stats_valid))
    return false;

  out->cpu_frame_ms = atomic_load(&context->stats.last_cpu_frame_ms);
  out->gpu_frame_ms = atomic_load(&context->stats.timestamps_supported)
                          ? atomic_load(&context->stats.last_gpu_frame_ms)
                          : 0.0f;
  out->frame_rate = atomic_load(&context->stats.last_frame_rate);

  f32 frame_budget_ms =
      context->target_refresh_rate > 0.0f
          ? 1000.0f / atomic_load(&context->target_refresh_rate)
          : (out->frame_rate > 0.0f ? 1000.0f / out->frame_rate : 16.7f);

  out->gpu_usage_pct = (frame_budget_ms > 0.0f)
                           ? (out->gpu_frame_ms / frame_budget_ms) * 100.0f
                           : 0.0f;
  if (out->gpu_usage_pct > 100.0f)
    out->gpu_usage_pct = 100.0f;

  out->vram_total = atomic_load(&context->stats.vram_total);
  out->vram_used = atomic_load(&context->stats.vram_used);
  out->vram_budget = atomic_load(&context->stats.vram_budget);
  out->memory_budget_valid = atomic_load(&context->stats.vram_budget_valid);

  return true;
}

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/aa.h>
#include <butter/internal/types.h>

/***********************************/

u32 butter_aa_resolve_samples(const butter_context_t *context) {
  if (context->aa_mode == BUTTER_AA_NONE)
    return 1;

  u32 wanted = context->aa_requested_samples;
  if (wanted == 0)
    wanted = context->aa_max_samples;

  u32 best = 1;
  for (u32 s = 1; s <= wanted; s <<= 1)
    if (context->aa_color_sample_counts & s)
      best = s;

  return best;
}

u32 butter_aa_resolve_budgeted(butter_context_t *context) {
  u32 samples = butter_aa_resolve_samples(context);
  if (samples <= 1 || context->aa_vram_budget == 0)
    return samples;

  u64 bytes_per_sample = (u64)context->extent.width *
                         (u64)context->extent.height * 4u *
                         (u64)context->image_count;
  if (bytes_per_sample == 0)
    return samples;

  u64 max_samples = context->aa_vram_budget / bytes_per_sample;
  while (samples > 1 && (u64)samples > max_samples)
    samples >>= 1;

  return samples;
}

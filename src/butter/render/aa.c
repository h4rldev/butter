/***********************************/

#include <threads.h>

#include <htils/basictypes.h>

#include <butter/internal/types.h>

#include <butter/log.h>
#include <butter/types.h>

#include <butter/render/aa.h>

/***********************************/

/**
 * @brief Resolve the effective MSAA sample count.
 * @details Clamps the requested sample count (or the device maximum when the
 * request is 0) down to the highest device-supported count. Returns 1 when
 * anti-aliasing is off.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 *
 * @return The resolved sample count.
 */
static u32 butter_aa_resolve_samples(const butter_t *butter) {
  if (butter->aa_mode == BUTTER_AA_NONE)
    return 1;

  u32 wanted = butter->aa_requested_samples;
  if (wanted == 0)
    wanted = butter->aa_max_samples;

  u32 best = 1;
  for (u32 s = 1; s <= wanted; s <<= 1)
    if (butter->aa_color_sample_counts & s)
      best = s;

  return best;
}

//
//
//

butter_aa_caps_t butter_get_aa_caps(butter_t *butter) {
  butter_aa_caps_t caps = {0};
  if (!butter)
    return caps;

  caps.color_sample_counts = butter->aa_color_sample_counts;
  caps.max_samples = butter->aa_max_samples;
  return caps;
}

void butter_set_aa_mode(butter_t *butter, butter_aa_mode_t mode) {
  if (!butter)
    return;

  if ((u32)mode >= BUTTER_AA_MODE_MAX)
    mode = BUTTER_AA_NONE;

  mtx_lock(&butter->aa_mutex);
  if (butter->aa_mode != (u32)mode) {
    butter->aa_mode = (u32)mode;
    butter->aa_samples = butter_aa_resolve_samples(butter);
    atomic_store(&butter->aa_dirty, true);
  }
  mtx_unlock(&butter->aa_mutex);
}

butter_aa_mode_t butter_get_aa_mode(butter_t *butter) {
  if (!butter)
    return BUTTER_AA_NONE;

  mtx_lock(&butter->aa_mutex);
  butter_aa_mode_t mode = (butter_aa_mode_t)butter->aa_mode;
  mtx_unlock(&butter->aa_mutex);
  return mode;
}

void butter_set_aa_samples(butter_t *butter, u32 samples) {
  if (!butter)
    return;

  mtx_lock(&butter->aa_mutex);
  if (butter->aa_requested_samples != samples) {
    butter->aa_requested_samples = samples;
    butter->aa_samples = butter_aa_resolve_samples(butter);
    atomic_store(&butter->aa_dirty, true);
  }
  mtx_unlock(&butter->aa_mutex);
}

u32 butter_get_aa_samples(butter_t *butter) {
  if (!butter)
    return 1;

  mtx_lock(&butter->aa_mutex);
  u32 samples = butter->aa_samples;
  mtx_unlock(&butter->aa_mutex);
  return samples;
}

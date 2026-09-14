/***********************************/

#include <errno.h> // IWYU pragma: keep
#include <threads.h>
#include <time.h>

#include <htils/basictypes.h>

#include <butter/render/pacing.h>

#include <butter/types.h>

/***********************************/

u64 get_time_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

void butter_limit_frame_rate(butter_t *butter, u64 frame_start_ns,
                             u64 present_end_ns) {
  if (!butter || butter->target_refresh_rate <= 0.0f)
    return;

  // No need to limit if vsync is enabled.
  if (atomic_load(&butter->vsync))
    return;

  u64 target_ns = (u64)(1e9f / butter->target_refresh_rate);
  u64 target_end_ns = frame_start_ns + target_ns;

  const u64 spin_tail_ns = 200000ULL;
  u64 sleep_until_ns =
      target_end_ns > spin_tail_ns ? target_end_ns - spin_tail_ns : 0;

  if (present_end_ns < sleep_until_ns) {
    struct timespec target = {
        .tv_sec = (time_t)(sleep_until_ns / 1000000000ULL),
        .tv_nsec = (long)(sleep_until_ns % 1000000000ULL),
    };

    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &target, null) ==
           EINTR)
      ;
  }

  while (get_time_ns() < target_end_ns)
    ;
}

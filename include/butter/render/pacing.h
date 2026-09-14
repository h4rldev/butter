#ifndef BUTTER_RENDER_PACING_H
#define BUTTER_RENDER_PACING_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/types.h>

/***********************************/

/**
 * @brief Get the current time in nanoseconds.
 * @details Uses clock_gettime to get the current time, then converts it to
 * nanoseconds.
 *
 * @return The current time in nanoseconds.
 */
u64 get_time_ns(void);

//
//
//

/**
 * @brief Limit the frame rate of the application to the target refresh rate.
 * @details If the target refresh rate is set, this function will limit the
 * render thread to the target refresh rate based on @c frame_start_ns, and @c
 * present_end_ns.
 *
 * @param butter The butter context.
 * @param frame_start_ns The start time of the frame in nanoseconds.
 * @param present_end_ns The end time of the present in nanoseconds.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c frame_start_ns must be a valid start time in nanoseconds.
 * - @c present_end_ns must be a valid end time in nanoseconds.
 */
void butter_limit_frame_rate(butter_t *butter, u64 frame_start_ns,
                             u64 present_end_ns);

#endif // !BUTTER_RENDER_PACING_H

#ifndef BUTTER_RENDER_STATS_H
#define BUTTER_RENDER_STATS_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/types.h>

/***********************************/

/**
 * @brief Get the latest renderer stats.
 * @details Populates @c out with the most recently measured frame times and
 * VRAM usage. Safe to call any time after at least one frame has been rendered.
 *
 * @param butter The butter context.
 * @param out    The stats to fill.
 *
 * @pre @c butter and @c out must be valid.
 *
 * @return true on success, false if the context is invalid or no frame has been
 *         rendered yet.
 */
b32 butter_get_stats(butter_t *butter, butter_stats_t *out);

#endif // !BUTTER_RENDER_STATS_H

#ifndef BUTTER_INTERNAL_AA_H
#define BUTTER_INTERNAL_AA_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>

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
u32 butter_aa_resolve_samples(const butter_context_t *context);

//
//
//

/**
 * @brief Resolve the MSAA sample count within the VRAM budget.
 * @details Calls @ref butter_aa_resolve_samples, then lowers the count while
 * the multisampled colour target for the context's current extent and image
 * count would exceed @c context->aa_vram_budget. Returns 1 when anti-aliasing
 * is off.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return The resolved sample count.
 */
u32 butter_aa_resolve_budgeted(butter_context_t *context);

#endif // !BUTTER_INTERNAL_AA_H

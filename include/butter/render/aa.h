#ifndef BUTTER_RENDER_AA_H
#define BUTTER_RENDER_AA_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Get the device's anti-aliasing capabilities.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 *
 * @return The anti-aliasing capabilities for this context.
 */
butter_aa_caps_t butter_get_aa_caps(butter_t *butter);

//
//
//

/**
 * @brief Set the anti-aliasing mode.
 *
 * @details Selects the anti-aliasing technique. The resolved sample count is
 * updated from @ref butter_set_aa_samples / the device maximum; the context
 * applies it when it (re)creates its render resources.
 *
 * @param butter The butter context.
 * @param mode The anti-aliasing mode.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_set_aa_mode(butter_t *butter, butter_aa_mode_t mode);

//
//
//

/**
 * @brief Get the anti-aliasing mode.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 *
 * @return The current anti-aliasing mode.
 */
butter_aa_mode_t butter_get_aa_mode(butter_t *butter);

//
//
//

/**
 * @brief Set the requested MSAA sample count.
 *
 * @details The value is clamped to the device-supported counts; 0 requests the
 * device maximum.
 *
 * @param butter The butter context.
 * @param samples The requested sample count, or 0 for the device maximum.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_set_aa_samples(butter_t *butter, u32 samples);

//
//
//

/**
 * @brief Get the resolved anti-aliasing sample count.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 *
 * @return The resolved sample count (1 when anti-aliasing is off).
 */
u32 butter_get_aa_samples(butter_t *butter);

#endif // !BUTTER_RENDER_AA_H

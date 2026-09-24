#ifndef BUTTER_RENDER_EFFECT_H
#define BUTTER_RENDER_EFFECT_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Run an effect into the current pass.
 * @details Binds @c effect->pipeline, allocates and fills its set 0 from
 * @c effect->inputs, pushes @c effect->push_constants, and draws the fullscreen
 * triangle. Effects are ordinary draws submitted in order: render into a
 * @ref butter_target_t, sample it with the next effect, and chain arbitrary
 * post-processing. A filter is an effect with one input; a composite is an
 * effect with several.
 *
 * @param butter The butter context.
 * @param effect The effect to run.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - A pass must be open (@ref butter_pass_begin).
 * - @c effect must be a valid effect with a pipeline.
 */
void butter_submit_effect(butter_t *butter, const butter_effect_t *effect);

#endif // !BUTTER_RENDER_EFFECT_H

#ifndef BUTTER_GRAPHICS_SAMPLER_H
#define BUTTER_GRAPHICS_SAMPLER_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Create a new sampler descriptor using a preset with linear clamping.
 *
 * @details Uses the linear clamping preset, with the following settings:
 * - Mag filter: VK_FILTER_LINEAR
 * - Min filter: VK_FILTER_LINEAR
 * - Mipmap mode: VK_SAMPLER_MIPMAP_MODE_LINEAR
 * - Address mode U: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
 * - Address mode V: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
 * - Address mode W: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
 * - Max anisotropy: 0.0f
 * - Compare enable: false
 * - Compare op: VK_COMPARE_OP_NEVER
 *
 * @return The new sampler descriptor.
 */
butter_sampler_desc_t butter_sampler_desc_linear_clamp(void);

//
//
//

/**
 * @brief Create a new sampler descriptor using a preset with linear repeat.
 *
 * @details Uses the linear repeat preset, with the following settings:
 * - Mag filter: VK_FILTER_LINEAR
 * - Min filter: VK_FILTER_LINEAR
 * - Mipmap mode: VK_SAMPLER_MIPMAP_MODE_LINEAR
 * - Address mode U: VK_SAMPLER_ADDRESS_MODE_REPEAT
 * - Address mode V: VK_SAMPLER_ADDRESS_MODE_REPEAT
 * - Address mode W: VK_SAMPLER_ADDRESS_MODE_REPEAT
 * - Max anisotropy: 0.0f
 * - Compare enable: false
 * - Compare op: VK_COMPARE_OP_NEVER
 *
 * @return The new sampler descriptor.
 */
butter_sampler_desc_t butter_sampler_desc_linear_repeat(void);

//
//
//

/**
 * @brief Create a new sampler descriptor using a preset with nearest clamping.
 *
 * @details Uses the nearest clamping preset, with the following settings:
 * - Mag filter: VK_FILTER_NEAREST
 * - Min filter: VK_FILTER_NEAREST
 * - Mipmap mode: VK_SAMPLER_MIPMAP_MODE_NEAREST
 * - Address mode U: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
 * - Address mode V: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
 * - Address mode W: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
 * - Max anisotropy: 0.0f
 * - Compare enable: false
 * - Compare op: VK_COMPARE_OP_NEVER
 *
 * @return The new sampler descriptor.
 */
butter_sampler_desc_t butter_sampler_desc_nearest_clamp(void);

//
//
//

/**
 * @brief Create a new sampler descriptor using a preset with anisotropic
 * filtering.
 *
 * @details Uses the anisotropic preset, with the following settings:
 * - Mag filter: VK_FILTER_LINEAR
 * - Min filter: VK_FILTER_LINEAR
 * - Mipmap mode: VK_SAMPLER_MIPMAP_MODE_LINEAR
 * - Address mode U: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
 * - Address mode V: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
 * - Address mode W: VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
 * - Max anisotropy: @c max_anisotropy
 * - Compare enable: false
 * - Compare op: VK_COMPARE_OP_NEVER
 *
 * @param max_anisotropy The max anisotropy.
 *
 * @pre
 * - @c max_anisotropy must be greater than 0.0f, if 0, it will be equal to
 * linear clamping.
 *
 * @return The new sampler descriptor.
 */
butter_sampler_desc_t butter_sampler_desc_anisotropic(f32 max_anisotropy);

//
//
//

/**
 * @brief Create a new sampler with the specified descriptor.
 *
 * @details Uses vkCreateSampler to create a new sampler with the provided
 * sampler descriptor.
 *
 * @param butter The butter context.
 * @param desc The sampler descriptor.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c desc must be a valid sampler descriptor.
 *
 * @return The new sampler.
 */
vk_sampler_t butter_create_sampler(butter_t *butter,
                                   const butter_sampler_desc_t *desc);

//
//
//

/**
 * @brief Destroy a sampler if it is not null.
 *
 * @details Uses vkDestroySampler to destroy the sampler.
 *
 * @param butter The butter context.
 * @param sampler The sampler to destroy.
 *
 * @pre
 * - @c butter must be a valid butter context (if null it's a no-op).
 * - @c sampler must be a valid sampler, (if null it's a no-op).
 */
void butter_destroy_sampler(butter_t *butter, vk_sampler_t sampler);

#endif // !BUTTER_GRAPHICS_SAMPLER_H

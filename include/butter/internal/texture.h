#ifndef BUTTER_INTERNAL_TEXTURE_H
#define BUTTER_INTERNAL_TEXTURE_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>

/***********************************/

#ifndef BUTTER_TEXTURE_REGISTRY_INITIAL
#define BUTTER_TEXTURE_REGISTRY_INITIAL (16)
#endif

//
//
//

/**
 * @brief Allocate and bind a texture's descriptor set.
 * @details A no-op when the device supports push descriptors (the image is
 * pushed per draw instead). Otherwise allocates a set from the context's
 * texture descriptor pool, growing it by a chunk when it is full, and writes
 * the texture's image view and sampler into the set.
 *
 * @param context The butter context.
 * @param texture The texture to create the descriptor set for.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c texture must have a created image view and sampler.
 *
 * @return True on success (or when push descriptors make it a no-op), false
 * otherwise.
 */
b32 butter_texture_create_descriptor_set(butter_context_t *context,
                                         struct butter_texture *texture);

//
//
//

/**
 * @brief Create the image and bind its memory for a texture.
 * @details Uses vkCreateImage and allocates device-local memory sized to the
 * texture's dimensions and format.
 *
 * @param context The butter context.
 * @param texture The texture to create the image for.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c texture must have @c width, @c height and @c format set.
 *
 * @return True if the image was created successfully, false otherwise.
 */
b32 butter_texture_create_image(butter_context_t *context,
                                struct butter_texture *texture,
                                vk_image_usage_flags_t usage);

//
//
//

/**
 * @brief Create the image view for a texture.
 *
 * @param context The butter context.
 * @param texture The texture to create the image view for.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c texture must have a created image.
 *
 * @return True if the image view was created successfully, false otherwise.
 */
b32 butter_texture_create_view(butter_context_t *context,
                               struct butter_texture *texture);

//
//
//

/**
 * @brief Initialize a sampleable, single-sample texture in place.
 * @details Creates the image, its view and its descriptor set for an
 * already-allocated texture struct, and marks it ready for sampling. Used by
 * render targets, whose texture is embedded and rebuilt in place.
 *
 * @param context The butter context.
 * @param texture The zeroed texture struct to initialize.
 * @param width The width.
 * @param height The height.
 * @param format The image format.
 * @param sampler The sampler shared by every draw sampling the texture.
 * @param usage The image usage flags.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c texture must point to a zeroed texture struct.
 * - @c width and @c height must be greater than 0.
 * - @c format must not be @c VK_FORMAT_UNDEFINED.
 * - @c sampler must be a valid sampler.
 *
 * @return True on success, false otherwise.
 */
b32 butter_texture_init_target(butter_context_t *context,
                               struct butter_texture *texture, u32 width,
                               u32 height, vk_format_t format,
                               vk_sampler_t sampler,
                               vk_image_usage_flags_t usage);

#endif // !BUTTER_INTERNAL_TEXTURE_H

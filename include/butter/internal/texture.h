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

#endif // !BUTTER_INTERNAL_TEXTURE_H

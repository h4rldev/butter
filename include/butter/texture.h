#ifndef BUTTER_TEXTURE_H
#define BUTTER_TEXTURE_H

/***********************************/

#include <butter/types.h>
#include <htils/basictypes.h>

/***********************************/

/**
 * @brief Create a new texture.
 * @details Creates a new texture and uploads it to the GPU, synchronously.
 *
 * @param butter The butter context.
 * @param width The width.
 * @param height The height.
 * @param format The format.
 * @param data The data.
 * @param data_size The size of the data.
 * @param sampler The sampler.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c width must be a valid width.
 * - @c height must be a valid height.
 * - @c format must be a valid format.
 * - @c data must be a valid pointer to @c data_size bytes.
 * - @c data_size must be greater than 0.
 * - @c sampler must be a valid sampler.
 *
 * @return The new texture.
 */
butter_texture_t *butter_create_texture(butter_t *butter, u32 width, u32 height,
                                        vk_format_t format, const void *data,
                                        u64 data_size, vk_sampler_t sampler);
//
//
//

/**
 * @brief Destroy a texture.
 * @details Destroys a texture, frees the memory and destroys the image view.
 *
 * @param butter The butter context.
 * @param texture The texture to destroy.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c texture must be a valid texture.
 */
void butter_destroy_texture(butter_t *butter, butter_texture_t *texture);

//
//
//

/**
 * @brief Initialize the texture upload queue.
 * @details Creates the async upload pool, then initializes the upload queue,
 * and runs the upload thread. If the texture upload thread is already running,
 * it's a no-op.
 *
 * @param butter The butter context.
 * @param queue_cap The queue capacity.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c queue_cap must be a valid queue capacity.
 */
void butter_init_texture_upload(butter_t *butter, u32 queue_cap);

//
//
//

/**
 * @brief Stop the texture upload queue.
 * @details Stops the texture upload queue, if it's not already stopped, it's a
 * no-op.
 *
 * @param butter The butter context.
 *
 * @pre @c butter must be a valid butter context.
 */
void butter_stop_texture_uploads(butter_t *butter);

//
//
//

/**
 * @brief Submit a texture for upload.
 * @details Submits a texture for upload, and returns the texture. If the upload
 * failed, it will be destroyed.
 *
 * @param butter The butter context.
 * @param width The width.
 * @param height The height.
 * @param format The format.
 * @param data The data.
 * @param data_size The size of the data.
 * @param sampler The sampler.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c width must be a valid width.
 * - @c height must be a valid height.
 * - @c format must be a valid format.
 * - @c data must be a valid pointer to @c data_size bytes.
 * - @c data_size must be greater than 0.
 * - @c sampler must be a valid sampler.
 *
 * @return The new texture that's been queued for upload.
 */
butter_texture_t *butter_submit_texture_upload(butter_t *butter, u32 width,
                                               u32 height, vk_format_t format,
                                               const void *data, u64 data_size,
                                               vk_sampler_t sampler);

//
//
//

/**
 * @brief Stop a texture upload.
 * @details Interrupts the upload of a texture, if it's not already cancelled,
 * its a simple no-op.
 *
 * @param butter The butter context.
 * @param texture The texture to stop uploading.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c texture must be a valid texture.
 */
void butter_stop_texture_upload(butter_t *butter, butter_texture_t *texture);

//
//
//

/**
 * @brief Update a sub-region of an existing texture.
 * @details Stages @c data_size bytes and asynchronously writes them into the
 * rectangle @c (x, y, w, h) of the texture, leaving the rest untouched. Data
 * must be tightly packed rows in the texture's format.
 *
 * @param butter The butter context.
 * @param texture The texture to update (image and view already created).
 * @param x The x offset of the region, in texels.
 * @param y The y offset of the region, in texels.
 * @param w The region width, in texels.
 * @param h The region height, in texels.
 * @param data The tightly packed pixel data for the region.
 * @param data_size The size of @c data in bytes; a size smaller than
 * @c w * @c h * bytes-per-pixel is logged as a warning.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c texture must be valid, with image and view created.
 * - @c data must point to @c data_size bytes.
 * - The region must be fully inside the texture.
 */
void butter_update_texture_region(butter_t *butter, butter_texture_t *texture,
                                  i32 x, i32 y, u32 w, u32 h, const void *data,
                                  u64 data_size);

//
//
//

/**
 * @brief Check if a texture is ready.
 * @details Checks if a texture is ready and has been uploaded.
 *
 * @param texture The texture to check.
 *
 * @pre @c texture must be a valid texture.
 *
 * @return true if the texture is ready, false otherwise.
 */
b32 butter_texture_is_ready(const butter_texture_t *texture);

//
//
//

/**
 * @brief Register a texture to the texture registry.
 * @details Registers a texture to the texture registry, and returns the
 * texture's id.
 *
 * @param butter The butter context.
 * @param texture The texture to register.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c texture must be a valid texture.
 *
 * @return The texture's id.
 */
i32 butter_texture_register(butter_t *butter, butter_texture_t *texture);

//
//
//

/**
 * @brief Deregister a texture from the texture registry.
 * @details Deregisters a texture from the texture registry, and destroys the
 * texture.
 *
 * @param butter The butter context.
 * @param id The id of the texture to deregister.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c id must be a valid id.
 */
void butter_texture_deregister(butter_t *butter, i32 id);

//
//
//

/**
 * @brief Get a texture from the texture registry.
 * @details Queries the texture registry for a texture with the given id.
 *
 * @param butter The butter context.
 * @param id The id of the texture to get.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c id must be a valid id.
 *
 * @return The texture with the given id, or null if no texture with the given
 * id is found.
 */
butter_texture_t *butter_texture_get(butter_t *butter, i32 id);

#endif // !BUTTER_TEXTURE_H

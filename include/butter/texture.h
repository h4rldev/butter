#ifndef BUTTER_TEXTURE_H
#define BUTTER_TEXTURE_H

#include <butter/types.h>
#include <htils/basictypes.h>

butter_texture_t butter_create_texture(butter_t *butter, u32 width, u32 height,
                                       vk_format_t format, const void *data,
                                       u64 data_size, vk_sampler_t sampler);
void butter_destroy_texture(butter_t *butter, butter_texture_t *texture);
void butter_init_texture_upload(butter_t *butter, u32 queue_cap);

void butter_stop_texture_uploads(butter_t *butter);
butter_texture_t butter_submit_texture_upload(butter_t *butter, u32 width,
                                              u32 height, vk_format_t format,
                                              const void *data, u64 data_size,
                                              vk_sampler_t sampler);
void butter_stop_texture_upload(butter_t *butter, butter_texture_t *texture);
b32 butter_texture_is_ready(const butter_texture_t *texture);

i32 butter_texture_register(butter_t *butter, butter_texture_t texture);

void butter_texture_deregister(butter_t *butter, i32 id);
butter_texture_t *butter_texture_get(butter_t *butter, i32 id);

#endif // !BUTTER_TEXTURE_H

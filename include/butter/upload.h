#ifndef BUTTER_UPLOAD_H
#define BUTTER_UPLOAD_H

#include <butter/types.h>
#include <htils/basictypes.h>

butter_texture_t butter_create_texture(butter_t *butter, u32 width, u32 height,
                                       vk_format_t format, const void *data,
                                       u64 data_size);

void butter_destroy_texture(butter_t *butter, butter_texture_t *texture);

void butter_init_upload(butter_t *butter, u32 queue_cap);

butter_texture_t butter_submit_texture_upload(butter_t *butter, u32 width,
                                              u32 height, vk_format_t format,
                                              const void *data, u64 data_size);

b32 butter_texture_is_ready(const butter_texture_t *texture);

void butter_stop_uploads(butter_t *butter);

void butter_stop_upload(butter_t *butter, butter_texture_t *texture);

#endif // !BUTTER_UPLOAD_H

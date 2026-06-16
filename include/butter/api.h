#ifndef BUTTER_API_H
#define BUTTER_API_H

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/types.h>

butter_t *butter_init(arena_t *arena, butter_surface_info_t *surface_info,
                      cstr *app_name, b32 use_validation_layers);
void butter_end(butter_t *butter);

void butter_set_clear_color(butter_t *butter, f32 r, f32 g, f32 b, f32 a);

butter_frame_t *butter_begin_frame(arena_t *arena, butter_t *butter);
vk_result_t butter_end_frame(arena_t *arena, butter_t *butter,
                             butter_frame_t *frame);

void butter_resize(arena_t *arena, butter_t *butter);

#endif // !BUTTER_API_H

#ifndef BUTTER_RENDER_H
#define BUTTER_RENDER_H

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/types.h>

butter_t *butter_init(arena_t *arena, butter_surface_info_t *surface_info,
                      cstr *app_name, b32 use_validation_layers, u32 width,
                      u32 height);
void butter_end(butter_t *butter);

void butter_set_clear_color(butter_t *butter, f32 r, f32 g, f32 b, f32 a);

butter_frame_t *butter_begin_frame(arena_t *arena, butter_t *butter);
vk_result_t butter_end_frame(arena_t *arena, butter_t *butter,
                             butter_frame_t *frame);

void butter_resize(arena_t *arena, butter_t *butter, u32 width, u32 height);

void butter_set_draw_callback(butter_t *butter, butter_draw_callback_t cb,
                              void *userdata);
void butter_start_render_thread(butter_t *butter, arena_t *per_frame_arena);
void butter_stop_render_thread(butter_t *butter);

void butter_request_frame(butter_t *butter);
void butter_wait_for_frame(butter_t *butter);

b32 butter_is_render_thread_running(const butter_t *butter);

void butter_set_vsync(butter_t *butter, b32 vsync);
void butter_set_target_refresh_rate(butter_t *butter, f32 rate);
f32 butter_get_target_refresh_rate(const butter_t *butter);

void butter_set_pending_resize(butter_t *butter, u32 width, u32 height);

#endif // !BUTTER_RENDER_H

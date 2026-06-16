#ifndef BUTTER_INTERNAL_GET_H
#define BUTTER_INTERNAL_GET_H

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

const cstr *const *
butter_get_required_instance_extensions(butter_backend_t backend, u32 *count);

vk_device_t butter_get_device(butter_context_t *context);

u32 butter_get_frame_index(butter_context_t *context);

u32 butter_get_image_count(butter_context_t *context);
vk_format_t butter_get_format(butter_context_t *context);
vk_extent2d_t butter_get_extent(butter_context_t *context);
vk_render_pass_t butter_get_default_render_pass(butter_context_t *context);
vk_framebuffer_t butter_get_framebuffer(butter_context_t *context,
                                        u32 image_index);
vk_queue_t butter_get_queue(butter_context_t *context);
u32 butter_get_queue_family(butter_context_t *context);
vk_command_pool_t butter_get_cmd_pool(butter_context_t *context);
vk_command_buffer_t butter_get_cmd(butter_context_t *context, u32 image_index);

#endif // !BUTTER_INTERNAL_GET_H

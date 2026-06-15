#ifndef BUTTER_SWAPCHAIN_H
#define BUTTER_SWAPCHAIN_H

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/types.h>

b32 butter_create_swapchain(arena_t *arena, butter_context_t *context);

vk_result_t butter_update_surface(arena_t *arena, butter_context_t *context);
vk_format_t butter_get_format(butter_context_t *context);
vk_extent2d_t butter_get_extent(butter_context_t *context);
vk_render_pass_t butter_get_default_render_pass(butter_context_t *context);
vk_framebuffer_t butter_get_framebuffer(butter_context_t *context,
                                        u32 image_index);
vk_queue_t butter_get_queue(butter_context_t *context);
u32 butter_get_queue_family(butter_context_t *context);

#endif // !BUTTER_SWAPCHAIN_H

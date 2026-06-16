#ifndef BUTTER_INIT_H
#define BUTTER_INIT_H

#include <htils/arena.h>

#include <butter/types.h>
#include <vulkan/vulkan.h>

vk_instance_t butter_create_instance(arena_t *arena, const cstr *app_name,
                                     b32 validation, butter_backend_t backend);
butter_context_t *butter_create(arena_t *arena, vk_instance_t instance,
                                const butter_surface_info_t *surface_info,
                                u32 latency_cap);
void butter_destroy(butter_context_t *context);

#endif // !BUTTER_INIT_H

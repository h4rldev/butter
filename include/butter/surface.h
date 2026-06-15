#ifndef BUTTER_SURFACE_H
#define BUTTER_SURFACE_H

#include <htils/arena.h>

#include <butter/types.h>
#include <vulkan/vulkan.h>

butter_context_t *butter_create(arena_t *arena, vk_instance_t instance,
                                const butter_surface_info_t *surface_info);

void butter_destroy(butter_context_t *context);

#endif // !BUTTER_SURFACE_H

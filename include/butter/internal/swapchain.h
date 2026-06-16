#ifndef BUTTER_SWAPCHAIN_H
#define BUTTER_SWAPCHAIN_H

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/types.h>

b32 butter_create_swapchain(arena_t *arena, butter_context_t *context,
                            u32 latency_cap);

vk_result_t butter_update_surface(arena_t *arena, butter_context_t *context,
                                  u32 latency_cap);

#endif // !BUTTER_SWAPCHAIN_H

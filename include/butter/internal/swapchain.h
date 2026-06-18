#ifndef BUTTER_SWAPCHAIN_H
#define BUTTER_SWAPCHAIN_H

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/types.h>

b32 butter_create_swapchain(arena_t *arena, butter_context_t *context,
                            u32 latency_cap, u32 desired_width,
                            u32 desired_height);

vk_result_t butter_update_surface(arena_t *arena, butter_context_t *context,
                                  u32 latency_cap, u32 desired_width,
                                  u32 desired_height);

#endif // !BUTTER_SWAPCHAIN_H

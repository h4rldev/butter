#ifndef BUTTER_INTERNAL_SWAPCHAIN_H
#define BUTTER_INTERNAL_SWAPCHAIN_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include <butter/types.h>

/***********************************/

/**
 * @brief Destroy the swapchain resources used by the context in order for
 * renew.
 * @details Destroys the framebuffers, depth images, depth image views, and
 * image views. Frees the depth memories.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 */
void butter_destroy_swapchain_resources(butter_context_t *context);

//
//
//

/**
 * @brief Create the swapchain.
 * @details Queries for capabilities and selects the most fit, then renews the
 * swapchain, if it's not available, it creates it.
 *
 * @param context The butter context.
 * @param latency_cap The latency cap.
 * @param desired_width The desired width.
 * @param desired_height The desired height.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c latency_cap must be a valid latency cap.
 * - @c desired_width must be a valid width.
 * - @c desired_height must be a valid height.
 *
 * @return true on success, false on error.
 */
b32 butter_create_swapchain(butter_context_t *context, u32 latency_cap,
                            u32 desired_width, u32 desired_height);

//
//
//

/**
 * @brief Update the render surface.
 * @details Updates the swapchain, if it's out of date, it destroys the old one.
 *
 * @param context The butter context.
 * @param latency_cap The latency cap.
 * @param desired_width The desired width.
 * @param desired_height The desired height.
 *
 * @pre
 * - @c context must be a valid butter context.
 * - @c latency_cap must be a valid latency cap.
 * - @c desired_width must be a valid width.
 * - @c desired_height must be a valid height.
 *
 * @return VK_SUCCESS on success, VK_ERROR_OUT_OF_DATE_KHR on out of date.
 */
vk_result_t butter_update_surface(butter_context_t *context, u32 latency_cap,
                                  u32 desired_width, u32 desired_height);

#endif // !BUTTER_INTERNAL_SWAPCHAIN_H

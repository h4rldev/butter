#ifndef BUTTER_INTERNAL_INIT_H
#define BUTTER_INTERNAL_INIT_H

/***********************************/

#include <htils/arena.h>

#include <butter/types.h>
#include <vulkan/vulkan.h>

/***********************************/

#ifndef BUTTER_LATENCY_CAP
#define BUTTER_LATENCY_CAP 4
#endif

//
//
//

/**
 * @brief Create a new vulkan instance.
 * @details Checks if Vulkan is available, and calculates the Vulkan API
 * version, gets the required extensions, and creates the instance with the
 * passed parameters.
 *
 * @param arena The arena to allocate the instance from.
 * @param app_name The name of the application.
 * @param validation Whether to enable validation layers.
 * @param backend The backend to use.
 *
 * @pre
 * - @c arena must be a valid arena.
 * - @c app_name must be a valid C-string.
 * - @c validation must be a valid validation flag.
 * - @c backend must be a valid backend.
 *
 * @return The new instance.
 */
vk_instance_t butter_create_instance(arena_t *arena, const cstr *app_name,
                                     b32 validation, butter_backend_t backend);

//
//
//

/**
 * @brief Create a new butter context.
 * @details Creates a new butter context with the given instance, surface info,
 * and configuration, initializing the required Vulkan objects.
 *
 * @param arena The arena to allocate the context from.
 * @param instance The Vulkan instance.
 * @param surface_info The surface info.
 * @param config The configuration.
 *
 * @pre
 * - @c arena must be a valid arena.
 * - @c instance must be a valid Vulkan instance.
 * - @c surface_info must be a valid surface info.
 * - @c config must be a valid configuration.
 *
 * @return The new context.
 */
butter_context_t *butter_create(arena_t *arena, vk_instance_t instance,
                                const butter_surface_info_t *surface_info,
                                const struct butter_init_config *config);

//
//
//

/**
 * @brief Destroy a butter context.
 * @details Destroys and frees all resources associated with the context.
 *
 * @param context The context to destroy.
 *
 * @pre
 * - @c context must be a valid context.
 */
void butter_destroy(butter_context_t *context);

#endif // !BUTTER_INTERNAL_INIT_H

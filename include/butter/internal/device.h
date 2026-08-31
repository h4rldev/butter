#ifndef BUTTER_INTERNAL_DEVICE_H
#define BUTTER_INTERNAL_DEVICE_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/internal/types.h>

/***********************************/

/**
 * @brief Select a physical device.
 * @details Queries the instance for the number of physical devices, then
 * selects the first one that supports graphics, then makes version checks to
 * see if the device supports the required features.
 *
 * @param arena The arena to allocate the physical devices from.
 * @param context The butter context.
 *
 * @pre
 * - @c arena must be a valid arena.
 * - @c context must be a valid butter context.
 *
 * @return true on success, false on error.
 */
b32 butter_select_physical_device(arena_t *arena, butter_context_t *context);

//
//
//

/**
 * @brief Create a device.
 * @details Creates a device with the selected physical device, queue family,
 * and enables support for the required features.
 *
 * @param context The butter context.
 *
 * @pre @c context must be a valid butter context.
 *
 * @return true on success, false on error.
 */
b32 butter_create_device(butter_context_t *context);

#endif // !BUTTER_INTERNAL_DEVICE_H

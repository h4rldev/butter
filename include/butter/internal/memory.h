#ifndef BUTTER_INTERNAL_MEMORY_H
#define BUTTER_INTERNAL_MEMORY_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>

/***********************************/

/**
 * @brief Find a suitable memory type.
 *
 * @details Uses vkGetPhysicalDeviceMemoryProperties to find a suitable memory
 * type within the physical device.
 *
 * @param physical_device The physical device to find a memory type for.
 * @param memory_type_bits The memory type bits to search for.
 * @param required_properties The required properties for the memory type.
 *
 * @pre
 * - @c physical_device must be a valid physical device.
 * - @c memory_type_bits must be a valid memory type bits bitmask.
 * - @c required_properties must be a valid memory property flags bitmask.
 *
 * @return The index of the suitable memory type.
 */
i32 butter_find_memory_type(vk_physical_device_t physical_device,
                            u32 memory_type_bits,
                            vk_memory_property_flags_t required_properties);

#endif // !BUTTER_INTERNAL_MEMORY_H

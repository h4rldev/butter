/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/log.h>

/***********************************/

i32 butter_find_memory_type(vk_physical_device_t physical_device,
                            u32 memory_type_bits,
                            vk_memory_property_flags_t required_properties) {
  vk_physical_device_memory_properties_t mem_props;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);

  for (u32 i = 0; i < mem_props.memoryTypeCount; i++)
    if ((memory_type_bits & (1 << i)) &&
        (mem_props.memoryTypes[i].propertyFlags & required_properties) ==
            required_properties)
      return i;

  butter_log_fatal("Failed to find suitable memory type");
  return -1;
}

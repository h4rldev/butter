#include <htils/basictypes.h>

#include <butter/internal/device.h>

b32 butter_select_physical_device(arena_t *arena, butter_context_t *context) {
  u32 count = 0;

  vkEnumeratePhysicalDevices(context->instance, &count, null);
  if (count == 0)
    return false;

  vk_physical_device_t *devices =
      arena_alloc(arena, vk_physical_device_t, count);
  vkEnumeratePhysicalDevices(context->instance, &count, devices);

  for (u32 i = 0; i < count; i++) {
    vk_physical_device_properties_t props;
    vkGetPhysicalDeviceProperties(devices[i], &props);

    u32 qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qf_count, null);
    vk_queue_family_properties_t *qf =
        arena_alloc(arena, vk_queue_family_properties_t, qf_count);
    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qf_count, qf);

    for (u32 j = 0; j < qf_count; j++) {
      if (qf[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        vk_bool32_t present = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j, context->surface,
                                             &present);
        if (present) {
          context->physical_device = devices[i];
          context->queue_family = j;
          return true;
        }
      }
    }
  }

  return false;
}

b32 butter_create_device(butter_context_t *context) {
  f32 prio = 1.0f;
  vk_device_queue_create_info_t device_queue_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = context->queue_family,
      .queueCount = 1,
      .pQueuePriorities = &prio,
  };

  const cstr *extensions[] = {
      "VK_KHR_swapchain",
  };

  vk_device_create_info_t device_create_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &device_queue_info,
      .enabledExtensionCount = 1,
      .ppEnabledExtensionNames = extensions,
  };

  if (vkCreateDevice(context->physical_device, &device_create_info, null,
                     &context->device) != VK_SUCCESS)
    return false;

  vkGetDeviceQueue(context->device, context->queue_family, 0, &context->queue);
  return true;
}

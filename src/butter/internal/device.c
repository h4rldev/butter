#include <htils/basictypes.h>

#include <butter/internal/device.h>
#include <butter/internal/types.h>

#include <butter/log.h>

b32 butter_select_physical_device(arena_t *arena, butter_context_t *context) {
  u32 count = 0;
  vk_result_t res;

  res = vkEnumeratePhysicalDevices(context->instance, &count, null);
  if (count == 0 || res != VK_SUCCESS) {
    butter_log_fatal("Could get physical device count");
    return false;
  }

  vk_physical_device_t *devices =
      arena_alloc_zeroed(arena, vk_physical_device_t, count);
  if ((res = vkEnumeratePhysicalDevices(context->instance, &count, devices)) !=
      VK_SUCCESS) {
    butter_log_fatal("Could not enumerate physical devices");
    return false;
  }

  for (u32 i = 0; i < count; i++) {
    vk_physical_device_properties_t props;
    vkGetPhysicalDeviceProperties(devices[i], &props);

    u32 qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qf_count, null);
    vk_queue_family_properties_t *qf =
        arena_alloc_zeroed(arena, vk_queue_family_properties_t, qf_count);
    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qf_count, qf);

    for (u32 j = 0; j < qf_count; j++) {
      if (qf[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        vk_bool32_t present = VK_FALSE;
        res = vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j,
                                                   context->surface, &present);
        if (res != VK_SUCCESS)
          butter_log_error("Could not get surface support");

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
  vk_result_t res;

  vk_device_queue_create_info_t device_queue_info = {0};
  device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  device_queue_info.queueFamilyIndex = context->queue_family;
  device_queue_info.queueCount = 1;
  device_queue_info.pQueuePriorities = &prio;

  const cstr *extensions[] = {
      "VK_KHR_swapchain",
  };

  vk_device_create_info_t device_create_info = {0};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.queueCreateInfoCount = 1;
  device_create_info.pQueueCreateInfos = &device_queue_info;
  device_create_info.enabledExtensionCount = 1;
  device_create_info.ppEnabledExtensionNames = extensions;

  if ((res = vkCreateDevice(context->physical_device, &device_create_info, null,
                            &context->device)) != VK_SUCCESS) {
    butter_log_fatal("Could not create device: %d", res);
    return false;
  }

  vkGetDeviceQueue(context->device, context->queue_family, 0, &context->queue);
  return true;
}

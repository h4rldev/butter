/***********************************/

#include <string.h>

#include <htils/basictypes.h>

#include <butter/internal/device.h>
#include <butter/internal/types.h>

#include <butter/log.h>

/***********************************/

b32 butter_select_physical_device(arena_t *arena, butter_context_t *context) {
  u32 count = 0;
  vk_result_t res;
  b32 found_q_pd = false;

  res = vkEnumeratePhysicalDevices(context->instance, &count, null);
  if (count == 0 || res != VK_SUCCESS) {
    butter_log_fatal("Could get physical device count");
    return false;
  }

  vk_physical_device_t *devices =
      arena_alloc_zeroed(arena, vk_physical_device_t, count);
  if (!devices) {
    butter_log_fatal("Could not allocate devices");
    return false;
  }

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
    if (!qf) {
      butter_log_fatal("Could not allocate qf");
      return false;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &qf_count, qf);

    for (u32 j = 0; j < qf_count; j++) {
      if (qf[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        vk_bool32_t present = VK_FALSE;
        res = vkGetPhysicalDeviceSurfaceSupportKHR(devices[i], j,
                                                   context->surface, &present);
        if (res != VK_SUCCESS) {
          butter_log_error("Could not get surface support");
          return false;
        }

        if (present) {
          context->physical_device = devices[i];
          context->queue_family = j;
          found_q_pd = true;
          break;
        }
      }
    }

    if (found_q_pd)
      break;
  }

  if (!found_q_pd)
    return false;

  u32 driver_version = context->driver_version;

  vk_physical_device_features2_t features2 = {0};
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

#ifdef VK_API_VERSION_1_1
  vk_physical_device_vulkan11_features_t f11 = {0};

  if (driver_version >= VK_API_VERSION_1_1) {
    f11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    features2.pNext = &f11;
  }
#endif

#ifdef VK_API_VERSION_1_2
  vk_physical_device_vulkan12_features_t f12 = {0};

  if (driver_version >= VK_API_VERSION_1_2) {
    f12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    f11.pNext = &f12;
  }
#endif

#ifdef VK_API_VERSION_1_3
  vk_physical_device_vulkan13_features_t f13 = {0};

  if (driver_version >= VK_API_VERSION_1_3) {
    f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    f12.pNext = &f13;
  }
#endif

#ifdef VK_API_VERSION_1_4
  vk_physical_device_vulkan14_features_t f14 = {0};
  if (driver_version >= VK_API_VERSION_1_4) {
    f14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
    f13.pNext = &f14;
  }
#endif

  vk_physical_device_present_wait_features_khr_t present_wait_features = {0};
  present_wait_features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;
  vk_physical_device_present_id_features_khr_t present_id_features = {0};
  present_id_features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
  present_id_features.pNext = features2.pNext;
  present_wait_features.pNext = &present_id_features;
  features2.pNext = &present_wait_features;

  vkGetPhysicalDeviceFeatures2(context->physical_device, &features2);
  context->available_vulkan_features = 0;

#ifdef VK_API_VERSION_1_2
  if (driver_version >= VK_API_VERSION_1_2 && f12.timelineSemaphore)
    context->available_vulkan_features |= BUTTER_FEATURE_TIMELINE_SEMAPHORE;
#endif

#ifdef VK_API_VERSION_1_3
  if (driver_version >= VK_API_VERSION_1_3) {
    if (f13.synchronization2)
      context->available_vulkan_features |= BUTTER_FEATURE_SYNCHRONIZATION_2;
  }
#endif

#ifdef VK_API_VERSION_1_4
  if (driver_version >= VK_API_VERSION_1_4 && f14.pushDescriptor)
    context->available_vulkan_features |= BUTTER_FEATURE_PUSH_DESCRIPTORS;
#endif

  u32 ext_count = 0;
  if (vkEnumerateDeviceExtensionProperties(context->physical_device, null,
                                           &ext_count, null) == VK_SUCCESS &&
      ext_count > 0) {
    vk_extension_properties_t *exts = arena_alloc_zeroed(
        context->arena, vk_extension_properties_t, ext_count);
    if (vkEnumerateDeviceExtensionProperties(context->physical_device, null,
                                             &ext_count, exts) == VK_SUCCESS) {
      b32 has_present_wait = false;
      b32 has_present_id = false;
      for (u32 i = 0; i < ext_count; i++) {
        if (strcmp(exts[i].extensionName,
                   VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0)
          context->available_vulkan_features |= BUTTER_FEATURE_MEMORY_BUDGET;
        else if (strcmp(exts[i].extensionName,
                        VK_KHR_PRESENT_WAIT_EXTENSION_NAME) == 0)
          has_present_wait = true;
        else if (strcmp(exts[i].extensionName,
                        VK_KHR_PRESENT_ID_EXTENSION_NAME) == 0)
          has_present_id = true;
      }
      if (has_present_wait && has_present_id &&
          present_wait_features.presentWait && present_id_features.presentId)
        context->available_vulkan_features |= BUTTER_FEATURE_PRESENT_WAIT;
    }
  }

  vk_physical_device_properties_t dev_props = {0};
  vkGetPhysicalDeviceProperties(context->physical_device, &dev_props);

  context->aa_color_sample_counts =
      (u32)(dev_props.limits.framebufferColorSampleCounts &
            dev_props.limits.sampledImageColorSampleCounts);
  context->aa_max_samples = 1;
  for (u32 s = 1; s <= 64; s <<= 1)
    if (context->aa_color_sample_counts & s)
      context->aa_max_samples = s;

  u32 qf_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device, &qf_count,
                                           null);
  if (qf_count > context->queue_family) {
    vk_queue_family_properties_t *qf_props =
        arena_alloc_zeroed(arena, vk_queue_family_properties_t, qf_count);
    vkGetPhysicalDeviceQueueFamilyProperties(context->physical_device,
                                             &qf_count, qf_props);
    atomic_store(&context->stats.timestamps_supported,
                 qf_props[context->queue_family].timestampValidBits > 0);
  }
  atomic_store(&context->stats.timestamp_period_ns,
               dev_props.limits.timestampPeriod);

  return true;
}

b32 butter_create_device(butter_context_t *context) {
  if (!context) {
    butter_log_error("Context is null");
    return false;
  }

  if (!context->physical_device) {
    butter_log_error("Physical device is null");
    return false;
  }

  if (context->queue_family == UINT32_MAX) {
    butter_log_error("Queue family is invalid");
    return false;
  }

  f32 prio = 1.0f;
  vk_result_t res;

  vk_physical_device_features2_t enable_features2 = {0};
  enable_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

  u32 driver_version = context->driver_version;

#ifdef VK_API_VERSION_1_1
  vk_physical_device_vulkan11_features_t enable_f11 = {0};
  if (driver_version >= VK_API_VERSION_1_1) {
    butter_log_debug("Enabling VK_API_VERSION_1_1 features");
    enable_f11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    enable_features2.pNext = &enable_f11;
  }
#endif

#ifdef VK_API_VERSION_1_2
  vk_physical_device_vulkan12_features_t enable_f12 = {0};
  if (driver_version >= VK_API_VERSION_1_2) {
    butter_log_debug("Enabling VK_API_VERSION_1_2 features");
    enable_f12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    enable_f12.timelineSemaphore =
        (context->available_vulkan_features & BUTTER_FEATURE_TIMELINE_SEMAPHORE)
            ? VK_TRUE
            : VK_FALSE;
    enable_f11.pNext = &enable_f12;
  }
#endif

#ifdef VK_API_VERSION_1_3
  vk_physical_device_vulkan13_features_t enable_f13 = {0};
  if (driver_version >= VK_API_VERSION_1_3) {
    butter_log_debug("Enabling VK_API_VERSION_1_3 features");
    enable_f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    enable_f13.synchronization2 =
        (context->available_vulkan_features & BUTTER_FEATURE_SYNCHRONIZATION_2)
            ? VK_TRUE
            : VK_FALSE;
    enable_f12.pNext = &enable_f13;
  }
#endif

#ifdef VK_API_VERSION_1_4
  vk_physical_device_vulkan14_features_t enable_f14 = {0};
  if (driver_version >= VK_API_VERSION_1_4) {
    butter_log_debug("Enabling VK_API_VERSION_1_4 features");
    enable_f14.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
    enable_f14.pushDescriptor =
        (context->available_vulkan_features & BUTTER_FEATURE_PUSH_DESCRIPTORS)
            ? VK_TRUE
            : VK_FALSE;
    enable_f13.pNext = &enable_f14;
  }
#endif

  vk_physical_device_present_wait_features_khr_t enable_present_wait = {0};
  vk_physical_device_present_id_features_khr_t enable_present_id = {0};
  if (context->available_vulkan_features & BUTTER_FEATURE_PRESENT_WAIT) {
    enable_present_wait.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR;
    enable_present_wait.presentWait = VK_TRUE;
    enable_present_id.sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR;
    enable_present_id.presentId = VK_TRUE;
    enable_present_id.pNext = enable_features2.pNext;
    enable_present_wait.pNext = &enable_present_id;
    enable_features2.pNext = &enable_present_wait;
  }

  vk_device_queue_create_info_t device_queue_info = {0};
  device_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  device_queue_info.queueFamilyIndex = context->queue_family;
  device_queue_info.queueCount = 1;
  device_queue_info.pQueuePriorities = &prio;

  const cstr *extensions[4];
  u32 extension_count = 0;
  extensions[extension_count++] = "VK_KHR_swapchain";
  if (context->available_vulkan_features & BUTTER_FEATURE_MEMORY_BUDGET)
    extensions[extension_count++] = VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
  if (context->available_vulkan_features & BUTTER_FEATURE_PRESENT_WAIT) {
    extensions[extension_count++] = VK_KHR_PRESENT_WAIT_EXTENSION_NAME;
    extensions[extension_count++] = VK_KHR_PRESENT_ID_EXTENSION_NAME;
  }

  vk_device_create_info_t device_create_info = {0};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.queueCreateInfoCount = 1;
  device_create_info.pQueueCreateInfos = &device_queue_info;
  device_create_info.enabledExtensionCount = extension_count;
  device_create_info.ppEnabledExtensionNames = extensions;
  device_create_info.pNext = &enable_features2;

  if ((res = vkCreateDevice(context->physical_device, &device_create_info, null,
                            &context->device)) != VK_SUCCESS) {
    butter_log_fatal("Could not create device: %d", res);
    return false;
  }

  vkGetDeviceQueue(context->device, context->queue_family, 0, &context->queue);
  if (context->available_vulkan_features & BUTTER_FEATURE_PRESENT_WAIT) {
    context->wait_for_present = (PFN_vkWaitForPresentKHR)vkGetDeviceProcAddr(
        context->device, "vkWaitForPresentKHR");
    if (!context->wait_for_present)
      context->available_vulkan_features &= ~BUTTER_FEATURE_PRESENT_WAIT;
  }

  return true;
}

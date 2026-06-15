#include <stdio.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#include "butter_internal.h"
#include <butter/check.h>
#include <butter/instance.h>

vk_instance_t butter_create_instance(arena_t *arena, const cstr *app_name,
                                     b32 validation, butter_backend_t backend) {
  if (butter_is_vulkan_available() == false) {
    fprintf(stderr, "Vulkan not available\n");
    return VK_NULL_HANDLE;
  }

  u32 extension_count = 0;
  const char *const *exts =
      butter_get_required_instance_extensions(backend, &extension_count);

  u32 total_ext_count = extension_count + (validation ? 1 : 0);
  const cstr **all_exts = arena_alloc(arena, const cstr *, total_ext_count);
  memcpy(all_exts, exts, sizeof(const cstr *) * extension_count);
  if (validation)
    all_exts[extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

  const cstr *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
  u32 layer_count = validation ? 1 : 0;

  vk_application_info_t app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = app_name,
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "butter",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_0,
  };

  vk_instance_create_info_t instance_create_info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &app_info,
      .enabledLayerCount = layer_count,
      .ppEnabledLayerNames = validation_layers,
      .enabledExtensionCount = total_ext_count,
      .ppEnabledExtensionNames = all_exts,
  };

  vk_instance_t instance = VK_NULL_HANDLE;
  vk_result_t res = vkCreateInstance(&instance_create_info, null, &instance);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "Could not create instance: %d\n", res);
    return VK_NULL_HANDLE;
  }

  return instance;
}

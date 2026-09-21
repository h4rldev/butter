/***********************************/

#include <dlfcn.h>

#include <butter/internal/check.h>
#include <htils/basictypes.h>

#include <butter/log.h>

/***********************************/

b32 butter_is_vulkan_available(void) {
  void *lib = dlopen("libvulkan.so.1", RTLD_NOW);
  if (!lib) {
    butter_log_fatal("Could not load libvulkan.so.1, is it installed or is the "
                     "so not versioned?");
    return false;
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

  pfn_vk_get_instance_proc_addr_t vkGetInstanceProcAddr =
      (pfn_vk_get_instance_proc_addr_t)dlsym(lib, "vkGetInstanceProcAddr");

  if (!vkGetInstanceProcAddr) {
    butter_log_fatal("Could not load vkGetInstanceProcAddr");
    return false;
  }

  pfn_vk_enumerate_instance_version_t vkEnumerateInstanceVersion =
      (pfn_vk_enumerate_instance_version_t)dlsym(lib,
                                                 "vkEnumerateInstanceVersion");

  b32 result = false;
  if (vkEnumerateInstanceVersion) {
    u32 version = 0;
    result = (vkEnumerateInstanceVersion(&version) == VK_SUCCESS);
  } else {
    pfn_vk_create_ínstance_t vkCreateInstance =
        (pfn_vk_create_ínstance_t)dlsym(lib, "vkCreateInstance");
    result = (vkCreateInstance != NULL);
  }

#pragma GCC diagnostic pop

  dlclose(lib);
  return result;
}

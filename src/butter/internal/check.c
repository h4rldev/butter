#include <dlfcn.h>

#include <htils/basictypes.h>

#include <butter/internal/check.h>
#include <butter/internal/types.h>

#include <butter/log.h>

b32 butter_is_vulkan_available(void) {
  void *lib = dlopen("libvulkan.so.1", RTLD_NOW);
  if (!lib) {
    butter_log_fatal("Could not load libvulkan.so.1, is it installed or is the "
                     "so not versioned?");
    return false;
  }

  pfn_vkGetInstanceProcAddr vkGetInstanceProcAddr =
      (pfn_vkGetInstanceProcAddr)dlsym(lib, "vkGetInstanceProcAddr");

  if (!vkGetInstanceProcAddr) {
    butter_log_fatal("Could not load vkGetInstanceProcAddr");
    return false;
  }

  pfn_vkEnumerateInstanceVersion vkEnumerateInstanceVersion =
      (pfn_vkEnumerateInstanceVersion)dlsym(lib, "vkEnumerateInstanceVersion");

  b32 result = false;
  if (vkEnumerateInstanceVersion) {
    u32 version = 0;
    result = (vkEnumerateInstanceVersion(&version) == VK_SUCCESS);
  } else {
    pfn_vkCreateInstance vkCreateInstance =
        (pfn_vkCreateInstance)dlsym(lib, "vkCreateInstance");
    result = (vkCreateInstance != NULL);
  }

  dlclose(lib);
  return result;
}

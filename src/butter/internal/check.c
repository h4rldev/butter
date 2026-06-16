#include <dlfcn.h>
#include <stdio.h>

#include <htils/basictypes.h>

#include <butter/internal/check.h>
#include <butter/internal/types.h>

b32 butter_is_vulkan_available(void) {
  void *lib = dlopen("libvulkan.so.1", RTLD_NOW);
  if (!lib) {
    fprintf(stderr, "Could not load libvulkan.so.1, could it be only available "
                    "unversioned?\n");
    return false;
  }

  pfn_vkGetInstanceProcAddr vkGetInstanceProcAddr =
      (pfn_vkGetInstanceProcAddr)dlsym(lib, "vkGetInstanceProcAddr");

  dlclose(lib);

  if (!vkGetInstanceProcAddr) {
    fprintf(stderr, "Could not load vkGetInstanceProcAddr\n");
    return false;
  }

  pfn_vkEnumerateInstanceVersion vkEnumerateInstanceVersion =
      (pfn_vkEnumerateInstanceVersion)dlsym(lib, "vkEnumerateInstanceVersion");

  if (vkEnumerateInstanceVersion) {
    u32 version = 0;
    return vkEnumerateInstanceVersion(&version) == VK_SUCCESS;
  }

  pfn_vkCreateInstance vkCreateInstance =
      (pfn_vkCreateInstance)dlsym(lib, "vkCreateInstance");
  return vkCreateInstance != NULL;
}

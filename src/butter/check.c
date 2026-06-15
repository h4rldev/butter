#include <dlfcn.h>
#include <stdio.h>

#include <htils/basictypes.h>

#include <butter/check.h>
#include <butter/types.h>

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

const cstr *const *
butter_get_required_instance_extensions(butter_backend_t backend, u32 *count) {
  static const char *xcb_exts[] = {
      "VK_KHR_surface",
      "VK_KHR_xcb_surface",
  };

  static const char *wayland_exts[] = {
      "VK_KHR_surface",
      "VK_KHR_wayland_surface",
  };

  switch (backend) {
  case BUTTER_BACKEND_XCB:
    *count = 2;
    return xcb_exts;
  case BUTTER_BACKEND_WAYLAND:
    *count = 2;
    return wayland_exts;
  default:
    *count = 0;
    return NULL;
  }
}

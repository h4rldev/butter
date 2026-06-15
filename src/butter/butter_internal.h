#ifndef BUTTER_INTERNAL_H
#define BUTTER_INTERNAL_H

#include <butter/types.h>
#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

#define BUTTER_MAX_FRAMES 3

struct butter_context {
  vk_instance_t instance;
  vk_surface_khr_t surface;
  butter_backend_t backend;

  vk_physical_device_t physical_device;
  vk_device_t device;
  vk_queue_t queue;
  u32 queue_family;

  vk_swapchain_khr_t swapchain;
  vk_format_t format;
  vk_extent2d_t extent;
  u32 image_count;
  vk_image_t *images;
  vk_image_view_t *image_views;
  vk_framebuffer_t *framebuffers;

  vk_render_pass_t render_pass;

  vk_fence_t in_flight_fences[BUTTER_MAX_FRAMES];
  vk_semaphore_t image_available[BUTTER_MAX_FRAMES];
  vk_semaphore_t rendering_finished[BUTTER_MAX_FRAMES];
  u32 frame_index;
};

#endif // !BUTTER_INTERNAL_H

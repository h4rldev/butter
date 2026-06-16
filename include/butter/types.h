#ifndef BUTTER_TYPES_H
#define BUTTER_TYPES_H

#include <butter/internal/types.h>

typedef struct butter_frame {
  vk_command_buffer_t cmd;
  vk_framebuffer_t fb;
  u32 image_index;
  vk_extent2d_t extent;
  vk_render_pass_t rp;
} butter_frame_t;

typedef enum butter_backend {
  BUTTER_BACKEND_XCB,
  BUTTER_BACKEND_WAYLAND,
} butter_backend_t;

typedef struct butter_surface_info {
  butter_backend_t backend;
  void *handle;
  void *display;
} butter_surface_info_t;

typedef butter_context_t butter_t;

#endif // !BUTTER_TYPES_H

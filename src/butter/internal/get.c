#include <htils/basictypes.h>

#include <butter/internal/get.h>
#include <butter/internal/types.h>

#include <butter/log.h>
#include <butter/types.h>

const cstr *const *
butter_get_required_instance_extensions(butter_backend_t backend, u32 *count) {
  if (!count) {
    butter_log_error("count must not be null");
    return null;
  }

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
    butter_log_fatal(
        "Incompatile platform, butter only supports xcb and wayland for now.");
    *count = 0;
    return NULL;
  }
}

vk_device_t butter_get_device(butter_context_t *context) {
  if (!context || !context->device) {
    butter_log_error("Context or device is null");
    return VK_NULL_HANDLE;
  }

  return context->device;
}

u32 butter_get_frame_index(butter_context_t *context) {
  if (!context) {
    butter_log_error("Context is null");
    return 0;
  }

  return context->frame_index;
}

u32 butter_get_image_count(butter_context_t *context) {
  if (!context) {
    butter_log_error("Context is null");
    return 0;
  }

  return context->image_count;
}

vk_format_t butter_get_format(butter_context_t *context) {
  if (!context) {
    butter_log_error("Context is null");
    return VK_FORMAT_UNDEFINED;
  }

  return context->format;
}

vk_extent2d_t butter_get_extent(butter_context_t *context) {
  if (!context) {
    butter_log_error("Context is null");
    return (vk_extent2d_t){0};
  }

  return context->extent;
}

vk_render_pass_t butter_get_default_render_pass(butter_context_t *context) {
  if (!context) {
    butter_log_error("Context is null");
    return VK_NULL_HANDLE;
  }

  return context->render_pass;
}

vk_framebuffer_t butter_get_framebuffer(butter_context_t *context,
                                        u32 image_index) {
  if (!context) {
    butter_log_error("Context is null");
    return VK_NULL_HANDLE;
  }

  if (image_index >= context->image_count) {
    butter_log_error("Image index out of bounds, got image index %d, but only "
                     "%d images exist",
                     image_index, context->image_count);
    return VK_NULL_HANDLE;
  }

  return context->framebuffers[image_index];
}

vk_queue_t butter_get_queue(butter_context_t *context) {
  if (!context || !context->queue) {
    butter_log_error("Context or queue is null");
    return VK_NULL_HANDLE;
  }

  return context->queue;
}
u32 butter_get_queue_family(butter_context_t *context) {
  if (!context) {
    butter_log_error("Context is null");
    return 0;
  }

  return context->queue_family;
}

vk_command_pool_t butter_get_cmd_pool(butter_context_t *context) {
  if (!context || !context->cmd_pool) {
    butter_log_error("Context or command pool is null");
    return VK_NULL_HANDLE;
  }

  return context->cmd_pool;
}

vk_command_buffer_t butter_get_cmd(butter_context_t *context, u32 image_index) {
  if (!context || !context->cmds) {
    butter_log_error("Context or the command buffers is null");
    return VK_NULL_HANDLE;
  }

  if (image_index >= context->image_count) {
    butter_log_error("Image index out of bounds, got image index %d, but only "
                     "%d images exist",
                     image_index, context->image_count);
    return VK_NULL_HANDLE;
  }

  return context->cmds[image_index];
}

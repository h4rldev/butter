/***********************************/

#include <threads.h>

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/internal/check.h>
#include <butter/internal/init.h>
#include <butter/internal/stats.h>

#include <butter/log.h>
#include <butter/types.h>

#include <butter/render/api.h>
#include <butter/render/thread.h>

/***********************************/

butter_init_config_t butter_init_config_default(void) {
  return (butter_init_config_t){
      .app_name = "butter",
      .use_validation_layers = false,
      .latency_cap = 0,
      .width = 0,
      .height = 0,
      .dynamic_vbo_size = 0,
      .dynamic_ibo_size = 0,
      .pipeline_cache_path = null,
      .enable_depth = false,
      .aa_mode = BUTTER_AA_NONE,
      .aa_samples = 0,
      .max_render_width = 0,
      .max_render_height = 0,
      .aa_vram_budget = 0,
  };
}

butter_t *butter_init(arena_t *arena, butter_surface_info_t *surface_info,
                      const butter_init_config_t *config) {
  butter_log_debug("Initializing butter");
  if (!arena || !surface_info || !config) {
    butter_log_error("Invalid arguments");
    return null;
  }

  vk_result_t res;
  butter_log_debug("Checking for vulkan support");
  if (!butter_is_vulkan_available())
    return null;

  butter_log_debug("Creating vulkan instance");
  vk_instance_t instance = butter_create_instance(arena, config->app_name,
                                                  config->use_validation_layers,
                                                  surface_info->backend);
  if (!instance) {
    butter_log_fatal("Could not create vulkan instance");
    return null;
  }

  butter_log_debug("Creating butter context");
  butter_t *butter = butter_create(arena, instance, surface_info, config);
  if (!butter) {
    butter_log_fatal("Could not create butter context");
    return null;
  }

  butter_log_debug("Creating command pool");
  vk_command_pool_create_info_t pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  pool_info.queueFamilyIndex = butter->queue_family;

  if ((res = vkCreateCommandPool(butter->device, &pool_info, null,
                                 &butter->cmd_pool)) != VK_SUCCESS) {
    butter_log_fatal("Could not create command pool: %d");
    butter_destroy(butter);
    return null;
  }

  butter->cmds =
      arena_alloc_zeroed(arena, vk_command_buffer_t, butter->frames_in_flight);

  vk_command_buffer_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = butter->cmd_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = butter->frames_in_flight;

  butter_log_debug("Allocating command buffers");
  if ((res = vkAllocateCommandBuffers(butter->device, &alloc_info,
                                      butter->cmds)) != VK_SUCCESS) {
    butter_log_fatal("Could not allocate command buffers: %d", res);
    vkDestroyCommandPool(butter->device, butter->cmd_pool, null);
    butter_destroy(butter);
    return null;
  }

  butter_stats_init(butter);

  return butter;
}

void butter_end(butter_t *butter) {
  if (!butter) {
    butter_log_debug("Butter context already destroyed");
    return;
  }
  vkDeviceWaitIdle(butter->device);
  vkDestroyCommandPool(butter->device, butter->cmd_pool, null);
  butter_destroy(butter);
}

void butter_set_clear_color(butter_t *butter, f32 r, f32 g, f32 b, f32 a) {
  butter->clear_color.color.float32[0] = r;
  butter->clear_color.color.float32[1] = g;
  butter->clear_color.color.float32[2] = b;
  butter->clear_color.color.float32[3] = a;
}

void butter_set_draw_callback(butter_t *butter, butter_draw_callback_t cb,
                              void *userdata) {
  if (!butter)
    return;
  butter->draw_callback = cb;
  butter->draw_userdata = userdata;
}

void butter_set_vsync(butter_t *butter, b32 vsync) {
  if (!butter)
    return;
  if (butter->vsync == vsync)
    return;

  mtx_lock(&butter->render_mutex);
  butter->vsync = vsync;
  butter->resize_pending = true;
  butter->pending_width = butter->extent.width;
  butter->pending_height = butter->extent.height;
  butter->swapchain_dirty = true;
  mtx_unlock(&butter->render_mutex);

  if (atomic_load(&butter->render_running)) {
    butter_request_frame(butter);
  }
}

void butter_set_target_refresh_rate(butter_t *butter, f32 rate) {
  if (!butter)
    return;
  butter->target_refresh_rate = rate;
}

f32 butter_get_target_refresh_rate(const butter_t *butter) {
  return butter ? butter->target_refresh_rate : 0.0f;
}

void butter_set_pending_resize(butter_t *butter, u32 width, u32 height) {
  if (!butter)
    return;
  mtx_lock(&butter->render_mutex);
  butter->pending_width = width;
  butter->pending_height = height;
  butter->resize_pending = true;
  mtx_unlock(&butter->render_mutex);
}

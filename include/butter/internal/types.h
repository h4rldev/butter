#ifndef BUTTER_INTERNAL_TYPES_H
#define BUTTER_INTERNAL_TYPES_H

/***********************************/

#include <threads.h>

#include <vulkan/vulkan.h>

#ifdef BUTTER_X11
#include <xcb/xcb.h>

#include <vulkan/vulkan_xcb.h>
#endif

#ifdef BUTTER_WAYLAND
#include <wayland-client.h>

#include <vulkan/vulkan_wayland.h>
#endif

#include <htils/arena.h>
#include <htils/atomic_types.h>
#include <htils/darray.h>

#include <butter/internal/vk_types.h>

/***********************************/

#define BUTTER_FEATURE_TIMELINE_SEMAPHORE (1 << 0)
#define BUTTER_FEATURE_SYNCHRONIZATION_2 (1 << 1)
#define BUTTER_FEATURE_PUSH_DESCRIPTORS (1 << 2)
#define BUTTER_FEATURE_MEMORY_BUDGET (1 << 3)
#define BUTTER_FEATURE_PRESENT_WAIT (1 << 4)

struct butter_pipeline_retained;

struct butter_pipeline {
  vk_pipeline_layout_t layout;
  vk_pipeline_t pipeline;
  b32 uses_descriptors;
  vk_descriptor_set_layout_t set_layout0;
  vk_shader_stage_flags_mask_t push_constant_stage_flags;
  struct butter_pipeline_retained *retained;
};

struct butter_frame {
  vk_command_buffer_t cmd;
  vk_framebuffer_t fb;
  vk_extent2d_t extent;
  vk_render_pass_t rp;
  u64 frame_start_ns;
  u32 image_index;
};

enum butter_shader_stage {
  BUTTER_STAGE_VERTEX,
  BUTTER_STAGE_TESSELLATION_CONTROL,
  BUTTER_STAGE_TESSELLATION_EVALUATION,
  BUTTER_STAGE_GEOMETRY,
  BUTTER_STAGE_FRAGMENT,
  BUTTER_STAGE_MAX
};

struct butter_shader {
  const cstr *name;
  enum butter_shader_stage stage;
  const void *code;
  const vk_specialization_info_t *spec;
  const cstr *entry_point;
  u64 code_size;
};

struct butter_buffer {
  vk_buffer_t handle;
  vk_device_memory_t memory;
  u64 size;
  void *mapped;
};

struct butter_descriptor_set {
  vk_descriptor_set_t set;
  vk_descriptor_set_layout_t layout;
};

struct butter_texture {
  vk_image_t image;
  vk_image_view_t view;
  vk_device_memory_t memory;
  u32 width;
  u32 height;
  vk_format_t format;
  struct butter_descriptor_set descriptor_set;
  vk_descriptor_pool_t descriptor_pool;
  vk_sampler_t sampler;

  atomic_b32 is_upload;
  atomic_b32 upload_ready;
  atomic_b32 upload_failed;
  atomic_b32 upload_cancelled;
};

struct butter_shader_registry {
  struct butter_shader *shaders;
  u32 capacity;
  u32 count;
};

struct butter_texture_registry_entry {
  struct butter_texture *texture;
  u32 id;
};

struct butter_texture_registry {
  struct butter_texture_registry_entry *entries;
  u32 capacity;
  u32 count;
  u32 next_id;
};

typedef struct {
  struct butter_texture *texture;
  struct butter_buffer staging_buffer;
  i32 offset_x;
  i32 offset_y;
  u32 region_width;
  u32 region_height;
  b32 ready;
  b32 failed;
  b32 cancelled;
} butter_upload_t;

struct butter_init_config {
  const cstr *app_name;
  b32 use_validation_layers;
  u32 latency_cap;
  u32 width;
  u32 height;
  u64 dynamic_vbo_size;
  u64 dynamic_ibo_size;
  const cstr *pipeline_cache_path;
  b32 enable_depth;
  u32 aa_mode;
  u32 aa_samples;
  u32 max_render_width;
  u32 max_render_height;
  u64 aa_vram_budget;
};

typedef void (*butter_draw_callback_t)(vk_command_buffer_t cmd,
                                       const struct butter_frame *frame,
                                       void *userdata);

typedef struct butter_stats_state {
  vk_query_pool_t timestamp_pool;
  atomic_f32 timestamp_period_ns;
  atomic_b32 timestamps_supported;
  atomic_b32 stats_valid;
  atomic_f32 last_cpu_frame_ms;
  atomic_f32 last_gpu_frame_ms;
  atomic_f32 last_frame_rate;
  atomic_u64 last_frame_start_ns;
  atomic_u64 vram_total;
  atomic_u64 vram_used;
  atomic_u64 vram_budget;
  atomic_b32 vram_budget_valid;
  atomic_u32 fps_frame_count;
  atomic_u64 fps_window_start_ns;
} butter_stats_state_t;

enum butter_aa_mode {
  BUTTER_AA_NONE,
  BUTTER_AA_MSAA,
  BUTTER_AA_MODE_MAX,
};

struct butter_target {
  u32 width;
  u32 height;
  u32 samples;
  b32 follow_width;
  b32 follow_height;
  vk_framebuffer_t framebuffer;
  struct butter_texture texture;
  vk_image_t msaa_image;
  vk_image_view_t msaa_view;
  vk_device_memory_t msaa_memory;
  vk_image_t depth_image;
  vk_image_view_t depth_view;
  vk_device_memory_t depth_memory;
  struct butter_target *next;
};

/**
 * @brief The butter render context.
 * @details Owns the Vulkan instance, device, surface, swapchain, and every
 * per-context resource. The draw callback, render thread, dynamic buffers,
 * texture/shader registries and stats all hang off it.
 *
 * The index spaces exist and are NOT interchangable, even though their arrays
 * are often the same length:
 * - @c in_flight_frame_slot: (0..frames_in_flight-1). Indexes per-frame
 * resources: cmds, in_flight_fences, image_available, and the dynamic
 * vbos/ibos. Advances each submitted frame, wrapping modulo frames_in_flight.
 * - @c image_index: (returned by acquire): the swapchain image actually being
 * rendered (0..image_count-1). Indexes per-image resources: images,
 * image_views, framebuffers, and rendering_finished (which gates present).
 *
 * @c frames_in_flight is derived (= min(latency_cap, image_count)); you cannot
 * render more frames ahead than there are swapchain images.
 */
typedef struct butter_context {
  vk_instance_t instance;
  u32 available_vulkan_features;
  u32 driver_version;

  vk_physical_device_t physical_device;
  vk_device_t device;
  vk_queue_t queue;
  vk_pipeline_cache_t pipeline_cache;
  u32 queue_family;

  vk_surface_khr_t surface;
  vk_swapchain_khr_t swapchain;
  vk_format_t format;
  vk_extent2d_t extent;
  u32 image_count;
  u32 frames_in_flight;
  u32 in_flight_frame_slot;

  const cstr *pipeline_cache_path;

  vk_image_t *images;
  vk_image_view_t *image_views;
  vk_framebuffer_t *framebuffers;

  vk_image_t *depth_images;
  vk_image_view_t *depth_image_views;
  vk_device_memory_t *depth_memories;
  b32 enable_depth;

  vk_render_pass_t render_pass;
  vk_render_pass_t render_pass_load;
  vk_render_pass_t render_pass_target;
  u32 render_pass_samples;

  vk_fence_t *in_flight_fences;
  vk_semaphore_t *image_available;
  vk_semaphore_t *rendering_finished;

  vk_semaphore_t timeline_semaphore;
  u64 timeline_value;

  u64 present_id;
  pfn_vk_wait_for_present_khr_t wait_for_present;
  b32 swapchain_fresh;

  vk_command_pool_t cmd_pool;
  vk_command_buffer_t *cmds;
  vk_clear_value_t clear_color;

  vk_descriptor_pool_t *effect_pools;
  u32 effect_pool_cap;

  vk_framebuffer_t pass_framebuffer;
  u32 pass_image_index;
  u32 pass_depth;

  struct butter_target *pass_target;
  vk_extent2d_t pass_extent;
  struct butter_target *targets;

  vk_present_mode_khr_t *available_modes;
  u32 available_mode_count;

  thrd_t render_thread;
  mtx_t render_mutex;
  cnd_t frame_ready;
  cnd_t frame_done;

  atomic_b32 render_running;
  atomic_b32 frame_requested;
  atomic_b32 frame_completed;
  atomic_b32 vsync;
  atomic_f32 target_refresh_rate;

  arena_t *arena;
  arena_t *render_arena;
  arena_t *swapchain_arena;
  arena_t *attachment_arena;

  butter_draw_callback_t draw_callback;
  void *draw_userdata;

  butter_stats_state_t stats;

  u32 pending_width;
  u32 pending_height;
  b32 resize_pending;
  b32 swapchain_dirty;

  thrd_t upload_thread;
  mtx_t upload_mutex;
  cnd_t upload_ready;
  b32 upload_thread_running;

  butter_upload_t *upload_queue;
  u32 upload_queue_cap;
  u32 upload_queue_head;
  u32 upload_queue_tail;

  vk_command_pool_t upload_pool_async;
  vk_command_pool_t upload_pool_sync;

  struct butter_buffer *dynamic_vbos;
  u32 dynamic_vbo_size;
  u64 dynamic_vbo_offset;

  struct butter_buffer *dynamic_ibos;
  u32 dynamic_ibo_size;
  u64 dynamic_ibo_offset;

  u32 dynamic_cap;

  struct butter_shader_registry *shader_registry;

  struct butter_pipeline **pipelines;
  u32 pipeline_count;
  u32 pipeline_cap;

  u32 aa_mode;
  u32 aa_samples;
  u32 aa_requested_samples;
  u32 aa_color_sample_counts;
  u32 aa_max_samples;
  atomic_b32 aa_dirty;
  mtx_t aa_mutex;

  u32 max_render_width;
  u32 max_render_height;
  u64 aa_vram_budget;

  vk_image_t *aa_color_images;
  vk_image_view_t *aa_color_image_views;
  vk_device_memory_t *aa_color_memories;

  struct butter_texture_registry texture_registry;
  vk_descriptor_pool_t *texture_descriptor_pools;
  u32 texture_descriptor_pool_count;
  mtx_t texture_descriptor_mutex;
  vk_descriptor_set_layout_t texture_descriptor_set_layout;
  vk_sampler_t default_sampler;
} butter_context_t;

#endif // !BUTTER_INTERNAL_TYPES_H

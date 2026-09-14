#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/file.h>
#include <htils/string.h>

#include <bread/backend.h>
#include <bread/event.h>
#include <bread/input.h>
#include <bread/window.h>

#include <vulkan/vulkan_core.h>
#include <xcb/xcb_icccm.h>

#include <butter/graphics.h>
#include <butter/log.h>
#include <butter/render.h>
#include <butter/types.h>

#define enable_validation true

typedef struct quad_vertex {
  f32 x, y;
  f32 u, v;
} quad_vertex_t;

#define FAN_HALF 0.8f
#define FAN_RADIUS 0.35f
#define FAN_EDGE_STEPS 4
#define FAN_ARC_STEPS 14

static quad_vertex_t fan_vertex(f32 x, f32 y) {
  return (quad_vertex_t){x, y, (x + FAN_HALF) / (2.0f * FAN_HALF),
                         (FAN_HALF - y) / (2.0f * FAN_HALF)};
}

static u32 build_rounded_fan(quad_vertex_t *verts) {
  const f32 pi = 3.14159265358979323846f;
  f32 core = FAN_HALF - FAN_RADIUS;
  u32 n = 0;

  verts[n++] = (quad_vertex_t){0.0f, 0.0f, 0.5f, 0.5f};

  for (u32 i = 0; i <= FAN_EDGE_STEPS; i++) {
    f32 t = (f32)i / (f32)FAN_EDGE_STEPS;
    verts[n++] = fan_vertex(-core + 2.0f * core * t, FAN_HALF);
  }
  for (u32 i = 1; i <= FAN_ARC_STEPS; i++) {
    f32 a = (pi / 2.0f) * (1.0f - (f32)i / (f32)FAN_ARC_STEPS);
    verts[n++] =
        fan_vertex(core + FAN_RADIUS * cosf(a), core + FAN_RADIUS * sinf(a));
  }
  for (u32 i = 1; i <= FAN_EDGE_STEPS; i++) {
    f32 t = (f32)i / (f32)FAN_EDGE_STEPS;
    verts[n++] = fan_vertex(FAN_HALF, core - 2.0f * core * t);
  }
  for (u32 i = 1; i <= FAN_ARC_STEPS; i++) {
    f32 a = -(pi / 2.0f) * ((f32)i / (f32)FAN_ARC_STEPS);
    verts[n++] =
        fan_vertex(core + FAN_RADIUS * cosf(a), -core + FAN_RADIUS * sinf(a));
  }
  for (u32 i = 1; i <= FAN_EDGE_STEPS; i++) {
    f32 t = (f32)i / (f32)FAN_EDGE_STEPS;
    verts[n++] = fan_vertex(core - 2.0f * core * t, -FAN_HALF);
  }
  for (u32 i = 1; i <= FAN_ARC_STEPS; i++) {
    f32 a = -(pi / 2.0f) - (pi / 2.0f) * ((f32)i / (f32)FAN_ARC_STEPS);
    verts[n++] =
        fan_vertex(-core + FAN_RADIUS * cosf(a), -core + FAN_RADIUS * sinf(a));
  }
  for (u32 i = 1; i <= FAN_EDGE_STEPS; i++) {
    f32 t = (f32)i / (f32)FAN_EDGE_STEPS;
    verts[n++] = fan_vertex(-FAN_HALF, -core + 2.0f * core * t);
  }
  for (u32 i = 1; i <= FAN_ARC_STEPS; i++) {
    f32 a = pi - (pi / 2.0f) * ((f32)i / (f32)FAN_ARC_STEPS);
    verts[n++] =
        fan_vertex(-core + FAN_RADIUS * cosf(a), core + FAN_RADIUS * sinf(a));
  }

  return n;
}

typedef struct app_state {
  butter_t *butter;
  bread_window_t *window;
  f32 r;
  f32 g;
  f32 b;

  butter_pipeline_t *pipeline;
  butter_buffer_t vertex_buffer;
  butter_buffer_t index_buffer;
  u32 index_count;
  f64 last_stats_print_s;
  b32 needs_redraw;
} app_state_t;

f32 random_f32(void) { return (f32)rand() / (f32)RAND_MAX; }

void bread_event_callback(bread_event_t *event, void *userdata) {
  app_state_t *data = (app_state_t *)userdata;

  switch (event->type) {
  case BREAD_EVENT_WINDOW_CLOSE:
    break;

  case BREAD_EVENT_MOUSE_MOVE:
    if (data->r > 1.0)
      data->r = 0.0;
    if (data->g > 1.0)
      data->g = 0.0;
    if (data->b > 1.0)
      data->b = 0.0;

    data->r = random_f32();
    data->g = random_f32();
    data->b = random_f32();
    data->needs_redraw = true;
    break;

  case BREAD_EVENT_KEY_PRESS: {
    u32 unicode = bread_event_key_to_unicode(data->window, event);
    if (unicode == 'a' || unicode == 'A') {
      u32 samples = butter_get_aa_samples(data->butter);
      u32 next = samples < 2 ? 2 : samples < 4 ? 4 : samples < 8 ? 8 : 1;
      butter_set_aa_mode(data->butter,
                         next > 1 ? BUTTER_AA_MSAA : BUTTER_AA_NONE);
      butter_set_aa_samples(data->butter, next);
      butter_log_info("AA: mode=%d samples=%u",
                      (i32)butter_get_aa_mode(data->butter),
                      butter_get_aa_samples(data->butter));
      data->needs_redraw = true;
      break;
    }

    fprintf(stderr, "Key: %i\n", event->data.key.key);
    cstr *key_cstring = bread_event_key_to_cstr(data->window, event);
    fprintf(stderr, "unicode: %d\n", unicode);
    fprintf(stderr, "key %s\n", key_cstring);
  } break;
  case BREAD_EVENT_WINDOW_RESIZE:
    butter_log_debug("width: %d, height: %d", event->data.resize.width,
                     event->data.resize.height);
    butter_set_pending_resize(data->butter, event->data.resize.width,
                              event->data.resize.height);
    data->needs_redraw = true;
    break;
  default:
    fprintf(stderr, "Unhandled event: %d\n", event->type);
    break;
  }
}

static app_state_t *create_quad(butter_t *butter) {
  arena_t *arena = butter->arena;
  string *vert =
      read_file(butter->arena, HTILS_STR("./src/test/quad.vert.spv"));
  string *frag =
      read_file(butter->arena, HTILS_STR("./src/test/quad.frag.spv"));

  if (!vert || !frag) {
    butter_log_fatal("Failed to load shaders");
    return null;
  }

  butter_shader_t shaders[2] = {
      {
          .stage = BUTTER_STAGE_VERTEX,
          .code = vert->base,
          .code_size = vert->len,
          .entry_point = "main",
      },
      {
          .stage = BUTTER_STAGE_FRAGMENT,
          .code = frag->base,
          .code_size = frag->len,
          .entry_point = "main",
      },
  };

  butter_attribute_t attributes[2] = {
      {
          .location = 0,
          .type = BUTTER_ATTRIB_POSITION_2D,
          .offset = offsetof(quad_vertex_t, x),
      },
      {
          .location = 1,
          .type = BUTTER_ATTRIB_UV,
          .offset = offsetof(quad_vertex_t, u),
      },
  };

  butter_pipeline_desc_t desc = butter_pipeline_desc_default();
  butter_pipeline_desc_add_shaders(&desc, shaders, 2);
  butter_pipeline_desc_add_attributes(&desc, attributes, 2);
  desc.cull_mode = BUTTER_CULL_NONE;
  desc.topology = BUTTER_TOPOLOGY_TRIANGLE_LIST;

  desc.descriptor_set_layouts = &butter->texture_descriptor_set_layout;
  desc.descriptor_set_layout_count = 1;

  butter_pipeline_t *pipeline = butter_create_pipeline(butter, &desc);
  if (!butter_pipeline_valid(pipeline)) {
    butter_log_fatal("Failed to create pipeline");
    return null;
  }

  quad_vertex_t *verts = arena_alloc_zeroed(arena, quad_vertex_t, 512);
  u32 *indices = arena_alloc_zeroed(arena, u32, 512 * 3);

  u32 vertex_count = build_rounded_fan(verts);
  u32 perimeter = vertex_count - 1;
  for (u32 i = 0; i < perimeter; i++) {
    indices[3 * i + 0] = 0;
    indices[3 * i + 1] = 1 + i;
    indices[3 * i + 2] = 1 + ((i + 1) % perimeter);
  }
  u32 index_count = perimeter * 3;

  butter_buffer_t vertex_buffer =
      butter_create_buffer(butter, vertex_count * sizeof(quad_vertex_t),
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
  if (vertex_buffer.handle == VK_NULL_HANDLE) {
    butter_log_fatal("Failed to create vertex buffer");
    return null;
  }

  butter_buffer_t index_buffer =
      butter_create_buffer(butter, index_count * sizeof(u32),
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true);
  if (index_buffer.handle == VK_NULL_HANDLE) {
    butter_log_fatal("Failed to create index buffer");
    return null;
  }

  memcpy(vertex_buffer.mapped, verts, vertex_count * sizeof(quad_vertex_t));
  memcpy(index_buffer.mapped, indices, index_count * sizeof(u32));

  app_state_t *resources = arena_alloc_zeroed(arena, app_state_t, 1);
  resources->pipeline = pipeline;
  resources->vertex_buffer = vertex_buffer;
  resources->index_buffer = index_buffer;
  resources->index_count = index_count;
  resources->butter = butter;
  return resources;
}

void draw_texture(vk_command_buffer_t cmd, const butter_frame_t *frame,
                  void *userdata) {
  app_state_t *state = (app_state_t *)userdata;

  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  double now_s = (double)now.tv_sec + (double)now.tv_nsec / 1e9;
  if (now_s - state->last_stats_print_s >= 1.0) {
    state->last_stats_print_s = now_s;
    butter_stats_t stats;
    if (butter_get_stats(state->butter, &stats))
      butter_log_info(
          "cpu=%.3fms gpu=%.3fms gpu_usage=%.1f%% fps=%.1f "
          "vram_used=%llu/%lluMiB budget=%lluMiB valid=%d",
          stats.cpu_frame_ms, stats.gpu_frame_ms, stats.gpu_usage_pct,
          stats.frame_rate, (u64)(stats.vram_used >> 20),
          (u64)(stats.vram_total >> 20), (u64)(stats.vram_budget >> 20),
          stats.memory_budget_valid);
  }
  butter_draw_cmd_t draw_cmd = {0};
  draw_cmd.pipeline = state->pipeline;
  draw_cmd.vertex_buffer = state->vertex_buffer.handle;
  draw_cmd.vertex_count = 0;
  draw_cmd.vertex_offset = 0;
  draw_cmd.index_buffer = state->index_buffer.handle;
  draw_cmd.index_count = state->index_count;
  draw_cmd.index_type = VK_INDEX_TYPE_UINT32;
  draw_cmd.texture_id = 0;

  butter_submit_draws(state->butter, &draw_cmd, 1);
}

int main(void) {
  arena_t *arena = arena_new(GiB(4), MiB(16));
  arena_t *frame_arena = arena_new(GiB(4), MiB(16));

  srand((unsigned)time(NULL));

  bread_window_t window = {
      .width = 800,
      .height = 600,
      .arena = arena,
  };

  bread_window_init(&window);

  bread_backend_type_t be = bread_get_backend_type();
  butter_backend_t backend =
      (be == BREAD_BACKEND_X11) ? BUTTER_BACKEND_XCB : BUTTER_BACKEND_WAYLAND;

  butter_surface_info_t surface_info = {
      .backend = backend,
      .handle = bread_window_get_surface(&window).handle,
      .display = bread_window_get_surface(&window).display,
  };

  butter_init_config_t config = butter_init_config_default();
  config.use_validation_layers = enable_validation;
  config.width = window.width;
  config.height = window.height;

  butter_t *butter = butter_init(arena, &surface_info, &config);
  if (!butter) {
    butter_log_error("Could not create context");
    return 1;
  }

  butter_set_clear_color(butter, 0.0f, 0.0f, 0.0f, 1.0f);

  butter_aa_caps_t aa_caps = butter_get_aa_caps(butter);
  butter_log_info("AA caps: sample_counts=0x%x max=%u (press A to cycle)",
                  aa_caps.color_sample_counts, aa_caps.max_samples);

  app_state_t *state = create_quad(butter);
  if (!state) {
    butter_log_error("Could not create quad");
    return 1;
  }

  state->butter = butter;
  state->window = &window;
  state->needs_redraw = true;

  butter_set_draw_callback(butter, draw_texture, state);

  bread_window_set_event_callback(&window, bread_event_callback, state);
  bread_window_set_min_size(&window, 600, 600);

  butter_set_vsync(butter, true);
  butter_start_render_thread(butter, frame_arena);
  while (bread_window_should_close(&window) == false) {
    bread_window_poll(&window);

    if (state->needs_redraw) {
      state->needs_redraw = false;

      butter_set_clear_color(butter, state->r, state->g, state->b, 1.0f);
      butter_request_frame(butter);

      while (!butter_frame_completed(butter)) {
        bread_window_poll(&window);
        thrd_sleep(&(struct timespec){.tv_nsec = 1000000}, null);
      }

      arena_clear(frame_arena);
    }
  }

  butter_stop_render_thread(butter);

  vkDeviceWaitIdle(butter->device);

  butter_destroy_pipeline(butter, state->pipeline);
  butter_destroy_buffer(butter, &state->vertex_buffer);
  butter_destroy_buffer(butter, &state->index_buffer);

  butter_end(butter);
  bread_window_destroy(&window);

  arena_free(frame_arena);
  arena_free(arena);

  return 0;
}

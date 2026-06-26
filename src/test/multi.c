#include <limits.h>
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

typedef struct {
  f32 x, y;
  f32 r, g, b, a;
} vertex_t;

static const vertex_t g_vertices[] = {
    {
        0.0f,
        0.5f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
    },
    {
        -0.5f,
        -0.5f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
    },
    {
        0.5f,
        -0.5f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
    },
};

typedef struct {
  butter_t *butter;
  bread_window_t *window;
  f32 r;
  f32 g;
  f32 b;
} bread_event_data_t;

f32 random_f32(void) { return (f32)rand() / (f32)RAND_MAX; }

void bread_event_callback(bread_event_t *event, void *userdata) {
  bread_event_data_t *data = (bread_event_data_t *)userdata;

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
    break;

  case BREAD_EVENT_KEY_PRESS: {
    fprintf(stderr, "Key: %i\n", event->data.key.key);
    u32 unicode = bread_event_key_to_unicode(data->window, event);
    cstr *key_cstring = bread_event_key_to_cstr(data->window, event);
    fprintf(stderr, "unicode: %d\n", unicode);
    fprintf(stderr, "key %s\n", key_cstring);

  } break;
  case BREAD_EVENT_WINDOW_RESIZE:
    butter_log_debug("width: %d, height: %d", event->data.resize.width,
                     event->data.resize.height);
    butter_set_pending_resize(data->butter, event->data.resize.width,
                              event->data.resize.height);
    break;
  default:
    break;
  }
}

typedef struct triangle_resources {
  butter_pipeline_t pipeline;
  butter_buffer_t vertex_buffer;
  butter_t *butter;
} triangle_resources_t;

static triangle_resources_t *create_triangle_resources(butter_t *butter) {
  arena_t *arena = butter->arena;
  string *vert =
      read_file(butter->arena, HTILS_STR("./src/test/triangle.vert.spv"));
  string *frag =
      read_file(butter->arena, HTILS_STR("./src/test/triangle.frag.spv"));

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
          .offset = offsetof(vertex_t, x),
      },
      {
          .location = 1,
          .type = BUTTER_ATTRIB_COLOR,
          .offset = offsetof(vertex_t, r),
      },
  };

  butter_pipeline_desc_t desc = butter_pipeline_desc_default();
  butter_pipeline_desc_add_shaders(&desc, shaders, 2);
  butter_pipeline_desc_add_attributes(&desc, attributes, 2);

  desc.cull_mode = BUTTER_CULL_NONE;

  butter_pipeline_t pipeline =
      butter_create_pipeline(butter, &desc, butter->render_pass);
  if (pipeline.pipeline == VK_NULL_HANDLE) {
    butter_log_fatal("Failed to create pipeline");
    return null;
  }

  butter_buffer_t vertex_buffer = butter_create_buffer(
      butter, sizeof(g_vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
  if (vertex_buffer.handle == VK_NULL_HANDLE) {
    butter_log_fatal("Failed to create vertex buffer");
    return null;
  }

  memcpy(vertex_buffer.mapped, g_vertices, sizeof(g_vertices));

  triangle_resources_t *resources =
      arena_alloc_zeroed(arena, triangle_resources_t, 1);
  resources->pipeline = pipeline;
  resources->vertex_buffer = vertex_buffer;
  resources->butter = butter;
  return resources;
}

void draw_triangle(vk_command_buffer_t cmd, const butter_frame_t *frame,
                   void *userdata) {
  triangle_resources_t *resources = (triangle_resources_t *)userdata;

  butter_draw_cmd_t draw_cmd = {0};
  draw_cmd.pipeline = resources->pipeline;
  draw_cmd.vertex_buffer = resources->vertex_buffer.handle;
  draw_cmd.vertex_count = 3;
  draw_cmd.vertex_offset = 0;
  draw_cmd.index_buffer = VK_NULL_HANDLE;
  draw_cmd.index_count = 0;

  butter_submit_draws(resources->butter, &draw_cmd, 1);
}

int main(void) {
  u32 current_frame_arena = 0;

  arena_t *arena = arena_new(GiB(4), MiB(16));

  arena_t *per_frame_arenas[2] = {
      arena_new(GiB(4), MiB(16)),
      arena_new(GiB(4), MiB(16)),
  };

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

  butter_t *butter = butter_init(arena, &surface_info, "butter", false,
                                 window.width, window.height);
  if (!butter) {
    butter_log_error("Could not create context");
    return 1;
  }

  butter_set_clear_color(butter, 0.0f, 0.0f, 0.0f, 1.0f);
  triangle_resources_t *resources = create_triangle_resources(butter);
  if (!resources) {
    butter_log_error("Could not create triangle resources");
    return 1;
  }

  bread_event_data_t *event_data = arena_alloc(arena, bread_event_data_t, 1);
  event_data->butter = butter;
  event_data->window = &window;

  butter_set_draw_callback(butter, draw_triangle, resources);

  bread_window_set_event_callback(&window, bread_event_callback, event_data);
  bread_window_set_min_size(&window, 800, 600);

  butter_set_vsync(butter, true);
  butter_set_target_refresh_rate(butter, 60.0f);

  butter_start_render_thread(butter, per_frame_arenas[current_frame_arena]);
  while (bread_window_should_close(&window) == false) {
    bread_window_poll(&window);

    butter_set_clear_color(butter, event_data->r, event_data->g, event_data->b,
                           1.0f);

    butter_request_frame(butter);
    // butter_wait_for_frame(butter);

    // butter_log_debug("Clearing per frame arena: %d", current_frame_arena);
    arena_clear(per_frame_arenas[current_frame_arena]);
    current_frame_arena = (current_frame_arena + 1) % 2;
  }

  butter_stop_render_thread(butter);

  vkDeviceWaitIdle(butter->device);

  butter_destroy_pipeline(butter, &resources->pipeline);
  butter_destroy_buffer(butter, &resources->vertex_buffer);

  butter_end(butter);
  bread_window_destroy(&window);

  arena_free(per_frame_arenas[0]);
  arena_free(per_frame_arenas[1]);
  arena_free(arena);

  return 0;
}

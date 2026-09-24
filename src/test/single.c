#include <stdio.h>

#include <htils/arena.h>
#include <htils/string.h>

#include <bread/event.h>
#include <bread/window.h>

#include <butter/render.h>

#define enable_validation true

typedef struct {
  bread_window_t *window;
  u32 pending_w;
  u32 pending_h;
  b32 resize_dirty;
} app_state_t;

static void print_rss(const char *tag) {
  FILE *f = fopen("/proc/self/smaps_rollup", "r");
  if (!f)
    return;
  char line[128];
  unsigned long rss = 0, pss = 0;
  while (fgets(line, sizeof line, f)) {
    if (sscanf(line, "Rss: %lu kB", &rss) == 1)
      continue;
    sscanf(line, "Pss: %lu kB", &pss);
  }
  fclose(f);
  fprintf(stderr, "%s: Rss=%.2f MiB Pss=%.2f MiB\n", tag, rss / 1024.0,
          pss / 1024.0);
}

void bread_event_callback(bread_event_t *event, void *userdata) {
  app_state_t *state = (app_state_t *)userdata;

  switch (event->type) {
  case BREAD_EVENT_WINDOW_RESIZE:
    state->pending_w = event->data.resize.width;
    state->pending_h = event->data.resize.height;
    state->resize_dirty = true;
    break;
  default:
    break;
  }
}

int main(void) {
  arena_t *arena = arena_new(GiB(4), MiB(256));

  bread_window_t window = {
      .width = 800,
      .height = 600,
      .title = HTILS_STR("butter test"),
      .arena = arena,
  };

  bread_window_init(&window);
  print_rss("bread");

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
    fprintf(stderr, "Could not create context\n");
    return 1;
  }

  print_rss("butter");

  app_state_t state = {.window = &window};
  bread_window_set_event_callback(&window, bread_event_callback, &state);
  butter_set_clear_color(butter, 0.2f, 0.3f, 0.8f, 1.0f);

  while (bread_window_should_close(&window) == false) {
    bread_window_poll(&window);

    if (state.resize_dirty) {
      butter_resize(butter, state.pending_w, state.pending_h);
      state.resize_dirty = false;
    }

    butter_frame_t *frame = butter_begin_frame(arena, butter);
    if (!frame) {
      state.pending_w = window.width;
      state.pending_h = window.height;
      state.resize_dirty = true;
      continue;
    }

    vk_result_t res = butter_end_frame(arena, butter, frame);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
      state.pending_w = window.width;
      state.pending_h = window.height;
      state.resize_dirty = true;
    }
  }

  butter_end(butter);
  bread_window_destroy(&window);
  arena_free(arena);

  return 0;
}

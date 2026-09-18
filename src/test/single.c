#include <stdio.h>

#include <htils/arena.h>
#include <htils/string.h>

#include <bread/backend.h>
#include <bread/event.h>
#include <bread/window.h>

#include <xcb/xcb_icccm.h>

#include <butter/render.h>

#define enable_validation false

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
  butter_t *butter = (butter_t *)userdata;

  // fprintf(stderr, "Got a event");
  switch (event->type) {
  case BREAD_EVENT_WINDOW_CLOSE:
    // fprintf(stderr, ", its a close event\n");
    break;
  case BREAD_EVENT_WINDOW_RESIZE:
    if (butter) {
      butter->pending_width = event->data.resize.width;
      butter->pending_height = event->data.resize.height;
      butter->resize_pending = true;
    }
    break;
  default:
    // fprintf(stderr, " of type %d\n", event->type);
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

  bread_window_set_event_callback(&window, bread_event_callback, butter);
  butter_set_clear_color(butter, 0.2f, 0.3f, 0.8f, 1.0f);
  while (bread_window_should_close(&window) == false) {
    bread_window_poll(&window);

    if (butter->resize_pending)
      butter_resize(butter, butter->pending_width, butter->pending_height);

    butter_frame_t *frame = butter_begin_frame(arena, butter);
    if (!frame) {
      if (butter->resize_pending) {
        butter_resize(butter, butter->pending_width, butter->pending_height);
        butter->resize_pending = false;
      }
      continue;
    }

    vk_result_t res = butter_end_frame(arena, butter, frame);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
      butter_resize(butter, butter->pending_width, butter->pending_height);
      butter->resize_pending = false;
    }
  }

  butter_end(butter);
  bread_window_destroy(&window);
  arena_free(arena);

  return 0;
}

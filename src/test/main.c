#include <stdio.h>
#include <string.h>

#include <htils/arena.h>
#include <htils/string.h>

#include <bread/backend.h>
#include <bread/event.h>
#include <bread/window.h>

#include <butter/api.h>

#define enable_validation true

void bread_event_callback(bread_event_t *event, void *userdata) {
  switch (event->type) {
  case BREAD_EVENT_WINDOW_CLOSE:
    fprintf(stderr, "Window got close event\n");
    break;
  default:
    fprintf(stderr, "Got event of type %d\n", event->type);
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

  bread_window_set_event_callback(&window, bread_event_callback, null);
  bread_window_init(&window);

  bread_backend_type_t be = bread_get_backend_type();
  butter_backend_t backend =
      (be == BREAD_BACKEND_X11) ? BUTTER_BACKEND_XCB : BUTTER_BACKEND_WAYLAND;

  butter_surface_info_t surface_info = {
      .backend = backend,
      .handle = bread_window_get_surface(&window).handle,
      .display = bread_window_get_surface(&window).display,
  };

  butter_t *butter = butter_init(arena, &surface_info, "butter", true);
  if (!butter) {
    fprintf(stderr, "Could not create context\n");
    return 1;
  }

  butter_set_clear_color(butter, 0.2f, 0.3f, 0.8f, 1.0f);
  while (bread_window_should_close(&window) == false) {
    bread_window_poll(&window);

    butter_frame_t *frame = butter_begin_frame(arena, butter);
    if (!frame)
      continue;

    vk_result_t res = butter_end_frame(arena, butter, frame);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
      butter_resize(arena, butter);
  }

  butter_end(butter);
  bread_window_destroy(&window);
  arena_free(arena);

  return 0;
}

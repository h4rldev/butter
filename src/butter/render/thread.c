/***********************************/

#include <threads.h>

#include <butter/internal/types.h>

#include <butter/types.h>

#include <butter/render/frame.h>
#include <butter/render/pacing.h>
#include <butter/render/thread.h>

/***********************************/

/**
 * @brief Render thread loop.
 * @details The render thread loop, waits for a frame to be requested, and then
 * submits and renders the frame.
 *
 * @param arg The render context.
 *
 * @pre @c arg cannot be empty.
 *
 * @return 0 on success, non-zero on error.
 */
static int render_thread_loop(void *arg) {
  butter_context_t *butter = (butter_context_t *)arg;

  while (atomic_load(&butter->render_running)) {
    mtx_lock(&butter->render_mutex);

    while (!atomic_load(&butter->frame_requested) &&
           atomic_load(&butter->render_running)) {
      cnd_wait(&butter->frame_ready, &butter->render_mutex);
    }

    if (!atomic_load(&butter->render_running)) {
      mtx_unlock(&butter->render_mutex);
      break;
    }

    atomic_store(&butter->frame_requested, false);
    mtx_unlock(&butter->render_mutex);

    if (butter->resize_pending) {
      u32 width = butter->pending_width;
      u32 height = butter->pending_height;
      butter->resize_pending = false;
      butter_resize(butter, width, height);
    }

    butter_frame_t *frame = butter_begin_frame(butter->render_arena, butter);
    if (frame) {
      if (butter->draw_callback)
        butter->draw_callback(frame->cmd, frame, butter->draw_userdata);

      vk_result_t res = butter_end_frame(butter->render_arena, butter, frame);
      if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        butter->pending_width = butter->extent.width;
        butter->pending_height = butter->extent.height;
        butter->resize_pending = true;
        butter->swapchain_dirty = true;
      }
    }

    mtx_lock(&butter->render_mutex);
    atomic_store(&butter->frame_completed, true);
    cnd_signal(&butter->frame_done);
    mtx_unlock(&butter->render_mutex);
  }
  return 0;
}

//
//
//

void butter_start_render_thread(butter_t *butter, arena_t *per_frame_arena) {
  if (!butter)
    return;
  if (butter->render_running)
    return;
  if (!per_frame_arena)
    return;

  butter->render_arena = per_frame_arena;
  atomic_store(&butter->render_running, true);
  atomic_store(&butter->frame_requested, false);
  atomic_store(&butter->frame_completed, false);
  butter->resize_pending = false;

  thrd_create(&butter->render_thread, render_thread_loop, butter);
}

void butter_stop_render_thread(butter_t *butter) {
  if (!butter || !atomic_load(&butter->render_running))
    return;

  mtx_lock(&butter->render_mutex);
  atomic_store(&butter->render_running, false);
  cnd_signal(&butter->frame_ready);
  mtx_unlock(&butter->render_mutex);

  thrd_join(butter->render_thread, null);
  atomic_store(&butter->render_running, false);
}

void butter_request_frame(butter_t *butter) {
  if (!butter || !atomic_load(&butter->render_running))
    return;

  mtx_lock(&butter->render_mutex);
  atomic_store(&butter->frame_requested, true);
  atomic_store(&butter->frame_completed, false);
  cnd_signal(&butter->frame_ready);
  mtx_unlock(&butter->render_mutex);
}

void butter_wait_for_frame(butter_t *butter) {
  if (!butter || !atomic_load(&butter->render_running))
    return;

  mtx_lock(&butter->render_mutex);
  while (!atomic_load(&butter->frame_completed))
    cnd_wait(&butter->frame_done, &butter->render_mutex);
  mtx_unlock(&butter->render_mutex);
}

b32 butter_is_render_thread_running(const butter_t *butter) {
  return butter ? (b32)atomic_load(&butter->render_running) : false;
}

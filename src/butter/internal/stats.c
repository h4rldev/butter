/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/stats.h>
#include <butter/internal/types.h>

#include <butter/log.h>

/***********************************/

#define BUTTER_STATS_QUERIES_PER_FRAME 2

//
//
//

/**
 * @brief Get the start of the timestamp query range for the given slot.
 *
 * @param slot The in-flight frame slot.
 * @return The start of the timestamp query range.
 */
static u64 stats_query_start(u32 slot) {
  return (u64)slot * BUTTER_STATS_QUERIES_PER_FRAME;
}

//
//
//

static void butter_stats_update_vram(butter_context_t *context) {
  if ((context->available_vulkan_features & BUTTER_FEATURE_MEMORY_BUDGET) == 0)
    return;

  vk_physical_device_memory_budget_properties_t budget = {0};
  budget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

  vk_physical_device_memory_properties2_t props2 = {0};
  props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
  props2.pNext = &budget;

  vkGetPhysicalDeviceMemoryProperties2(context->physical_device, &props2);

  u64 total = 0, used = 0, budget_total = 0;
  for (u32 i = 0; i < props2.memoryProperties.memoryHeapCount; i++) {
    vk_memory_heap_t *heap = &props2.memoryProperties.memoryHeaps[i];
    if (heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
      total += heap->size;
      used += budget.heapUsage[i];
      budget_total += budget.heapBudget[i];
    }
  }

  atomic_store(&context->stats.vram_total, total);
  atomic_store(&context->stats.vram_used, used);
  atomic_store(&context->stats.vram_budget, budget_total);
  atomic_store(&context->stats.vram_budget_valid, true);
}

//
//
//

void butter_stats_init(butter_context_t *context) {
  if (!context || !context->device)
    return;

  if (!atomic_load(&context->stats.timestamps_supported))
    return;

  vk_result_t res;
  vk_query_pool_create_info_t pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
  pool_info.queryCount =
      context->frames_in_flight * BUTTER_STATS_QUERIES_PER_FRAME;

  if ((res = vkCreateQueryPool(context->device, &pool_info, null,
                               &context->stats.timestamp_pool)) != VK_SUCCESS) {
    butter_log_warning(
        "Couldn't create timestamp query pool; GPU timing disabled: %d", res);
    return;
  }

  vk_command_buffer_allocate_info_t alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = context->cmd_pool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = 1;

  vk_command_buffer_t reset_cmd = VK_NULL_HANDLE;
  if (vkAllocateCommandBuffers(context->device, &alloc_info, &reset_cmd) ==
      VK_SUCCESS) {
    vk_command_buffer_begin_info_t begin = {0};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(reset_cmd, &begin) == VK_SUCCESS) {
      vkCmdResetQueryPool(reset_cmd, context->stats.timestamp_pool, 0,
                          pool_info.queryCount);
      if (vkEndCommandBuffer(reset_cmd) == VK_SUCCESS) {
        vk_submit_info_t submit = {0};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &reset_cmd;
        if (vkQueueSubmit(context->queue, 1, &submit, VK_NULL_HANDLE) ==
            VK_SUCCESS)
          vkQueueWaitIdle(context->queue);
      }
    }
    vkFreeCommandBuffers(context->device, context->cmd_pool, 1, &reset_cmd);
  }
}

void butter_stats_destroy(butter_context_t *context) {
  if (!context || !context->device)
    return;

  if (context->stats.timestamp_pool) {
    vkDestroyQueryPool(context->device, context->stats.timestamp_pool, null);
    context->stats.timestamp_pool = VK_NULL_HANDLE;
  }
}

void butter_stats_begin_frame(butter_context_t *context,
                              vk_command_buffer_t cmd, u32 slot) {
  if (!context || !cmd)
    return;

  vk_query_pool_t pool = context->stats.timestamp_pool;
  if (!pool)
    return;

  u64 ts[2] = {0, 0};
  if (vkGetQueryPoolResults(context->device, pool, (u32)stats_query_start(slot),
                            BUTTER_STATS_QUERIES_PER_FRAME, sizeof(ts), ts,
                            sizeof(u64),
                            VK_QUERY_RESULT_64_BIT) == VK_SUCCESS) {
    if (ts[1] > ts[0]) {
      f64 period = atomic_load(&context->stats.timestamp_period_ns);
      u64 ns = (u64)((f64)(ts[1] - ts[0]) * period);
      atomic_store(&context->stats.last_gpu_frame_ms, (f32)((f64)ns / 1e6f));
    }
  }

  vkCmdResetQueryPool(cmd, pool, (u32)stats_query_start(slot),
                      BUTTER_STATS_QUERIES_PER_FRAME);
  vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, pool,
                      (u32)stats_query_start(slot));
}

void butter_stats_end_timestamp(butter_context_t *context,
                                vk_command_buffer_t cmd, u32 slot) {
  if (!context || !cmd)
    return;

  vk_query_pool_t pool = context->stats.timestamp_pool;
  if (pool)
    vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, pool,
                        stats_query_start(slot) + 1);
}

void butter_stats_record_cpu(butter_context_t *context, f32 cpu_frame_ms) {
  if (!context)
    return;

  atomic_store(&context->stats.last_cpu_frame_ms, cpu_frame_ms);
  atomic_store(&context->stats.stats_valid, true);
  butter_stats_update_vram(context);
}

void butter_stats_tick_fps(butter_context_t *context, u64 now_ns) {
  u64 window_start = atomic_load(&context->stats.fps_window_start_ns);
  if (window_start == 0) {
    atomic_store(&context->stats.fps_window_start_ns, now_ns);
    return;
  }

  u32 count = atomic_fetch_add(&context->stats.fps_frame_count, 1) + 1;
  u64 elapsed = now_ns - window_start;
  if (elapsed >= 500000000ULL) {
    atomic_store(&context->stats.last_frame_rate,
                 (f32)((f64)count * 1e9 / (f64)elapsed));
    atomic_store(&context->stats.fps_frame_count, 0);
    atomic_store(&context->stats.fps_window_start_ns, now_ns);
  }
}

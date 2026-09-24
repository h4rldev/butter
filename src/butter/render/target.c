/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/target.h>
#include <butter/internal/types.h>

#include <butter/render/target.h>

#include <butter/log.h>
#include <butter/types.h>

/***********************************/

butter_target_t *butter_target_create(butter_t *butter, arena_t *arena,
                                      const butter_target_desc_t *desc) {
  if (!butter || !arena || !desc) {
    butter_log_error("Invalid arguments for render target");
    return null;
  }

  u32 width = desc->width ? desc->width : butter->extent.width;
  u32 height = desc->height ? desc->height : butter->extent.height;
  if (width == 0 || height == 0) {
    butter_log_error("Render target size is zero");
    return null;
  }

  butter_target_t *target = arena_alloc_zeroed(arena, butter_target_t, 1);
  target->width = width;
  target->height = height;
  target->follow_width = desc->width == 0;
  target->follow_height = desc->height == 0;

  if (!butter_target_build(butter, target))
    return null;

  target->next = butter->targets;
  butter->targets = target;

  return target;
}

void butter_target_destroy(butter_t *butter, butter_target_t *target) {
  if (!butter || !target) {
    butter_log_error("Invalid arguments for render target destroy");
    return;
  }

  struct butter_target **link = &butter->targets;
  while (*link && *link != target)
    link = &(*link)->next;
  if (*link)
    *link = target->next;
  target->next = null;

  butter_target_teardown(butter, target);
}

butter_texture_t *butter_target_texture(butter_target_t *target) {
  if (!target)
    return null;
  return &target->texture;
}

void butter_pass_begin(butter_t *butter, butter_target_t *target,
                       b32 color_load, b32 depth_load) {
  if (!butter) {
    butter_log_error("Butter instance not initialized");
    return;
  }

  if (butter->pass_depth > 0) {
    butter_log_error("Cannot nest render passes");
    return;
  }

  vk_framebuffer_t framebuffer;
  vk_extent2d_t extent;
  b32 depth;
  if (target) {
    framebuffer = target->framebuffer;
    extent = (vk_extent2d_t){target->width, target->height};
    depth = target->depth_image != VK_NULL_HANDLE;
  } else {
    if (butter->pass_framebuffer == VK_NULL_HANDLE) {
      butter_log_error("No active frame to begin a pass on");
      return;
    }

    framebuffer = butter->pass_framebuffer;
    extent = butter->extent;
    depth = butter->enable_depth;
  }

  b32 load = color_load;
  if (target) {
    if (color_load)
      butter_log_warning("Render targets always clear; ignoring load");

    load = false;
  } else {
    if (depth_load != color_load)
      butter_log_warning("Depth load follows the colour choice; ignoring");

    if (load && butter->render_pass_load == VK_NULL_HANDLE) {
      butter_log_warning("No load render pass available; clearing");
      load = false;
    }
  }

  vk_command_buffer_t cmd = butter->cmds[butter->in_flight_frame_slot];
  if (!cmd) {
    butter_log_error("No command buffer available");
    return;
  }

  vk_render_pass_begin_info_t rp_begin = {0};
  rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp_begin.renderPass =
      target ? butter->render_pass_target
             : (load ? butter->render_pass_load : butter->render_pass);
  rp_begin.framebuffer = framebuffer;
  rp_begin.renderArea.extent = extent;
  rp_begin.renderArea.offset = (vk_offset2d_t){0};

  vk_clear_value_t clears[2] = {
      butter->clear_color,
      (vk_clear_value_t){.depthStencil = {1.0f, 0}},
  };

  rp_begin.clearValueCount = depth ? 2 : 1;
  rp_begin.pClearValues = clears;

  vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
  butter->pass_target = target;
  butter->pass_extent = extent;
  butter->pass_depth++;
}

void butter_pass_end(butter_t *butter) {
  if (!butter || butter->pass_depth == 0) {
    butter_log_error("No render pass to end");
    return;
  }

  vkCmdEndRenderPass(butter->cmds[butter->in_flight_frame_slot]);
  butter->pass_depth--;
}

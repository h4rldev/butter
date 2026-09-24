/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>

#include <butter/graphics/descriptor.h>
#include <butter/graphics/draw.h>
#include <butter/graphics/pipeline.h>

#include <butter/render/effect.h>

#include <butter/log.h>
#include <butter/texture.h>
#include <butter/types.h>

/***********************************/

void butter_submit_effect(butter_t *butter, const butter_effect_t *effect) {
  if (!butter || !effect) {
    butter_log_error("Invalid arguments for effect");
    return;
  }

  if (!effect->pipeline || !butter_pipeline_valid(effect->pipeline)) {
    butter_log_error("Effect has no valid pipeline");
    return;
  }

  if (butter->pass_depth == 0) {
    butter_log_error("Cannot submit an effect outside a render pass");
    return;
  }

  butter_draw_cmd_t cmd = {0};
  cmd.pipeline = effect->pipeline;
  cmd.vertex_count = 3;
  cmd.push_constants = effect->push_constants;
  cmd.push_constant_size = effect->push_constant_size;

  if (effect->input_count > 0) {
    if (!effect->inputs) {
      butter_log_error("Effect has no inputs");
      return;
    }

    if (effect->pipeline->set_layout0 == VK_NULL_HANDLE) {
      butter_log_error("Effect pipeline has no set 0 layout");
      return;
    }

    for (u32 i = 0; i < effect->input_count; i++)
      if (!effect->inputs[i]) {
        butter_log_error("Effect input %d is null", i);
        return;
      }

    if (butter->available_vulkan_features & BUTTER_FEATURE_PUSH_DESCRIPTORS) {
      cmd.input_textures = effect->inputs;
      cmd.input_texture_count = effect->input_count;
    } else {
      if (!butter->effect_pools ||
          butter->in_flight_frame_slot >= butter->effect_pool_cap) {
        butter_log_error("No effect descriptor pool available");
        return;
      }

      butter_descriptor_set_t set = butter_allocate_descriptor_set(
          butter, butter->effect_pools[butter->in_flight_frame_slot],
          effect->pipeline->set_layout0);
      if (set.set == VK_NULL_HANDLE)
        return;

      for (u32 i = 0; i < effect->input_count; i++)
        butter_update_descriptor_image(butter, &set, i, effect->inputs[i]->view,
                                       effect->inputs[i]->sampler);

      cmd.descriptor_sets = &set;
      cmd.descriptor_set_count = 1;
    }
  }

  butter_submit_draws(butter, &cmd, 1);
}

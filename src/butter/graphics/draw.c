/***********************************/

#include <string.h>

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/texture.h>
#include <butter/types.h>

/***********************************/

void butter_submit_draws(butter_t *butter, const butter_draw_cmd_t *cmds,
                         u32 count) {
  if (!butter || !cmds || count == 0) {
    butter_log_error("Invalid arguments");
    return;
  }

  vk_command_buffer_t cmd = butter->cmds[butter->frame_index];
  if (!cmd) {
    butter_log_error("No command buffer available");
    return;
  }

  vk_viewport_t viewport = {0};
  viewport.x = 0.0f;
  viewport.y = (f32)butter->extent.height;
  viewport.width = (f32)butter->extent.width;
  viewport.height = -(f32)butter->extent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vkCmdSetViewport(cmd, 0, 1, &viewport);

  vk_pipeline_t bound_pipeline = VK_NULL_HANDLE;
  vk_pipeline_layout_t bound_layout = VK_NULL_HANDLE;
  vk_descriptor_set_t bound_set = VK_NULL_HANDLE;
  b32 scissor_set = false;
  vk_rect2d_t bound_scissor = {0};

  for (u32 i = 0; i < count; i++) {
    const butter_draw_cmd_t *draw = &cmds[i];

    if (draw->pipeline.pipeline == VK_NULL_HANDLE) {
      butter_log_error("Draw command has no pipeline");
      continue;
    }

    vk_rect2d_t scissor =
        draw->scissor_enabled
            ? draw->scissor
            : (vk_rect2d_t){.offset = {0, 0}, .extent = butter->extent};

    if (!scissor_set ||
        memcmp(&scissor, &bound_scissor, sizeof(scissor)) != 0) {
      vkCmdSetScissor(cmd, 0, 1, &scissor);
      bound_scissor = scissor;
      scissor_set = true;
    }

    if (draw->pipeline.pipeline != bound_pipeline) {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        draw->pipeline.pipeline);
      bound_pipeline = draw->pipeline.pipeline;
    }

    if (draw->pipeline.layout != bound_layout) {
      bound_layout = draw->pipeline.layout;
      bound_set = VK_NULL_HANDLE;
    }

    if (draw->vertex_buffer)
      vkCmdBindVertexBuffers(cmd, 0, 1, &draw->vertex_buffer,
                             &draw->vertex_offset);

    if (draw->pipeline.uses_descriptors) {
      if (draw->descriptor_sets && draw->descriptor_set_count > 0) {
        vk_descriptor_set_t sets[draw->descriptor_set_count];
        for (u32 j = 0; j < draw->descriptor_set_count; j++)
          sets[j] = draw->descriptor_sets[j].set;

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                draw->pipeline.layout, 0,
                                draw->descriptor_set_count, sets, 0, null);
      } else {
        butter_texture_t *tex = null;
        if (draw->texture_id != 0)
          tex = butter_texture_get(butter, draw->texture_id);

        if (!tex || !butter_texture_is_ready(tex))
          tex = butter_texture_get(butter, 0);

        if (tex) {
          if (butter->available_vulkan_features &
              BUTTER_FEATURE_PUSH_DESCRIPTORS) {
            vk_descriptor_image_info_t image_info = {0};
            image_info.imageView = tex->view;
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.sampler = tex->sampler;

            vk_write_descriptor_set_t write = {0};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstBinding = 0;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &image_info;

            vkCmdPushDescriptorSet(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   draw->pipeline.layout, 0, 1, &write);
          } else if (tex->descriptor_set.set != bound_set) {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    draw->pipeline.layout, 0, 1,
                                    &tex->descriptor_set.set, 0, null);
            bound_set = tex->descriptor_set.set;
          }
        }
      }
    }

    if (draw->index_buffer && draw->index_count > 0) {
      vkCmdBindIndexBuffer(cmd, draw->index_buffer, draw->index_offset,
                           draw->index_type);
      vkCmdDrawIndexed(cmd, draw->index_count, 1, 0, 0, 0);
    } else
      vkCmdDraw(cmd, draw->vertex_count, 1, 0, 0);
  }
}

butter_allocation_t butter_alloc_vertices(butter_t *butter, u32 vertex_count,
                                          u32 stride) {
  if (!butter || vertex_count == 0 || stride == 0) {
    butter_log_error("Invalid arguments");
    return (butter_allocation_t){0};
  }

  u32 frame_index = butter->frame_index;
  butter_allocation_t allocation = {0};

  u64 size_needed = (u64)vertex_count * stride;

  if (butter->dynamic_vbo_offset + size_needed > butter->dynamic_vbo_size) {
    butter_log_error("Dynamic buffer overflow");
    return allocation;
  }

  butter_buffer_t *buffer = &butter->dynamic_vbos[frame_index];
  allocation.buffer = buffer->handle;
  allocation.offset = butter->dynamic_vbo_offset;
  allocation.mapped = (u8 *)buffer->mapped + butter->dynamic_vbo_offset;

  butter->dynamic_vbo_offset += size_needed;

  return allocation;
}

butter_allocation_t butter_alloc_indices(butter_t *butter, u32 index_count,
                                         vk_index_type_t index_type) {
  u32 stride = 0;
  switch (index_type) {
  case VK_INDEX_TYPE_UINT16:
    stride = sizeof(u16);
    break;
  case VK_INDEX_TYPE_UINT32:
    stride = sizeof(u32);
    break;
  case VK_INDEX_TYPE_UINT8:
    stride = sizeof(u8);
    break;
  default:
    butter_log_error("Unknown index type: %d", index_type);
    return (butter_allocation_t){0};
  }

  return butter_alloc_vertices(butter, index_count, stride);
}

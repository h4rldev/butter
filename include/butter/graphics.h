#ifndef BUTTER_GRAPHICS_H
#define BUTTER_GRAPHICS_H

#include <butter/internal/types.h>
#include <butter/types.h>

butter_pipeline_desc_t butter_pipeline_desc_default(void);
void butter_pipeline_desc_add_shaders(butter_pipeline_desc_t *desc,
                                      butter_shader_t *shaders,
                                      u64 shader_count);
void butter_pipeline_desc_add_attributes(butter_pipeline_desc_t *desc,
                                         butter_attribute_t *attributes,
                                         u32 attribute_count);
void butter_pipeline_desc_set_vertex_stride(butter_pipeline_desc_t *desc,
                                            u32 stride);

butter_pipeline_t butter_create_pipeline(butter_t *butter,
                                         const butter_pipeline_desc_t *desc,
                                         vk_render_pass_t render_pass);

void butter_destroy_pipeline(butter_t *butter, butter_pipeline_t *pipeline);

butter_buffer_t butter_create_buffer(butter_t *butter, u64 size,
                                     vk_buffer_usage_flags_t usage,
                                     b32 host_visible);

void butter_destroy_buffer(butter_t *butter, butter_buffer_t *buffer);

vk_descriptor_pool_t
butter_create_descriptor_pool(butter_t *butter, u32 max_sets,
                              const vk_descriptor_pool_size_t *pool_sizes,
                              u32 pool_size_count);

void butter_destroy_descriptor_pool(butter_t *butter,
                                    vk_descriptor_pool_t pool);
vk_descriptor_set_layout_t butter_create_descriptor_set_layout(
    butter_t *butter, vk_descriptor_set_layout_binding_t *bindings,
    u32 binding_count);

butter_descriptor_set_t
butter_allocate_descriptor_set(butter_t *butter, vk_descriptor_pool_t pool,
                               vk_descriptor_set_layout_t layout);

void butter_update_descriptor_buffer(butter_t *butter,
                                     butter_descriptor_set_t *set, u32 binding,
                                     vk_buffer_t buffer, u64 offset, u64 size);

void butter_update_descriptor_image(butter_t *butter,
                                    butter_descriptor_set_t *set, u32 binding,
                                    vk_image_view_t image_view,
                                    vk_sampler_t sampler);

butter_sampler_desc_t butter_sampler_desc_linear_clamp(void);
butter_sampler_desc_t butter_sampler_desc_linear_repeat(void);
butter_sampler_desc_t butter_sampler_desc_nearest_clamp(void);
butter_sampler_desc_t butter_sampler_desc_anisotropic(f32 max_anisotropy);

vk_sampler_t butter_create_sampler(butter_t *butter,
                                   const butter_sampler_desc_t *desc);

void butter_destroy_sampler(butter_t *butter, vk_sampler_t sampler);

void butter_submit_draws(butter_t *butter, const butter_draw_cmd_t *cmds,
                         u32 count);

butter_allocation_t butter_alloc_vertices(butter_t *butter, u32 vertex_count,
                                          u32 stride);

#endif // !BUTTER_GRAPHICS_H

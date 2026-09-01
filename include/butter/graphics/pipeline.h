#ifndef BUTTER_GRAPHICS_PIPELINE_H
#define BUTTER_GRAPHICS_PIPELINE_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Create a pipeline descriptor with default values.
 * @details Initializes the descriptor with the following settings:
 * - Topology: BUTTER_TOPOLOGY_TRIANGLE_LIST
 * - Polygon mode: BUTTER_POLYGON_MODE_FILL
 * - Cull mode: BUTTER_CULL_BACK
 * - Blend mode: BUTTER_BLEND_NONE
 * - Front face: BUTTER_FRONT_FACE_CLOCKWISE
 * - Depth test: true
 * - Depth write: true
 * - Vertex stride: 0
 *
 * @return The default pipeline descriptor.
 */
butter_pipeline_desc_t butter_pipeline_desc_default(void);

//
//
//

/**
 * @brief Add shaders to a butter pipeline descriptor.
 *
 * @details Generic setter for pipeline descriptor fields, adding shaders.
 *
 * @param desc The pipeline descriptor to add shaders to.
 * @param shaders The shaders to add.
 * @param shader_count The amount of shaders.
 *
 * @pre
 * - @c desc must be a valid pipeline descriptor.
 * - @c shaders must be a valid pointer to an array of shaders.
 * - @c shader_count must be greater than 0 and match the size of the shaders
 * array ptr.
 */

void butter_pipeline_desc_add_shaders(butter_pipeline_desc_t *desc,
                                      butter_shader_t *shaders,
                                      u64 shader_count);

//
//
//

/**
 * @brief Set descriptor set layouts for a butter pipeline descriptor.
 *
 * @details Generic setter for pipeline descriptor set layout fields, adding
 * descriptor set layouts.
 *
 * @param desc The pipeline descriptor to add descriptor set layouts to.
 * @param layouts The descriptor set layouts to add.
 * @param layout_count The amount of descriptor set layouts.
 *
 * @pre
 * - @c desc must be a valid pipeline descriptor.
 * - @c layouts must be a valid pointer to an array of descriptor set layouts.
 * - @c layout_count must be greater than 0 and match the size of the layouts
 * array ptr.
 */
void butter_pipeline_desc_add_descriptor_set_layouts(
    butter_pipeline_desc_t *desc, vk_descriptor_set_layout_t *layouts,
    u32 layout_count);

//
//
//

/**
 * @brief Set attributes for a butter pipeline descriptor.
 *
 * @details Sets attributes for a pipeline descriptor, and calculates the vertex
 * stride.
 *
 * @param desc The pipeline descriptor to add attributes to.
 * @param attributes The attributes to add.
 * @param attribute_count The amount of attributes.
 *
 * @pre
 * - @c desc must be a valid pipeline descriptor.
 * - @c attributes must be a valid pointer to an array of attributes.
 * - @c attribute_count must be greater than 0 and match the size of the
 * attributes array ptr.
 */
void butter_pipeline_desc_add_attributes(butter_pipeline_desc_t *desc,
                                         butter_attribute_t *attributes,
                                         u32 attribute_count);

//
//
//

/**
 * @brief Set the vertex stride for a butter pipeline descriptor.
 *
 * @details Generic setter for pipeline descriptor vertex stride field.
 *
 * @param desc The pipeline descriptor to set the vertex stride for.
 * @param stride The vertex stride.
 *
 * @pre
 * - @c desc must be a valid pipeline descriptor.
 * - @c stride must be greater than 0.
 */
void butter_pipeline_desc_set_vertex_stride(butter_pipeline_desc_t *desc,
                                            u32 stride);

//
//
//

/**
 * @brief Create a new render pipeline.
 *
 * @details Uses vkCreateGraphicsPipelines to create a new graphics pipeline
 * with the descriptor passed and the render pass provided.
 *
 * @param butter The butter context.
 * @param desc The pipeline descriptor.
 * @param render_pass The render pass to use.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c desc must be a valid pipeline descriptor.
 * - @c render_pass must be a valid render pass.
 *
 * @return The new pipeline.
 */
butter_pipeline_t butter_create_pipeline(butter_t *butter,
                                         const butter_pipeline_desc_t *desc,
                                         vk_render_pass_t render_pass);

//
//
//

/**
 * @brief Destroy a pipeline and it's layout if it is not null.
 *
 * @details Uses vkDestroyPipeline to destroy the graphics pipeline and
 * vkDestroyPipelineLayout to destroy the pipeline layout.
 *
 * @param butter The butter context.
 * @param pipeline The pipeline to destroy.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c pipeline must be a valid pipeline.
 */
void butter_destroy_pipeline(butter_t *butter, butter_pipeline_t *pipeline);

#endif // !BUTTER_GRAPHICS_PIPELINE_H

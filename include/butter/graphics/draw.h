#ifndef BUTTER_GRAPHICS_DRAW_H
#define BUTTER_GRAPHICS_DRAW_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Submit draw commands to the command buffer.
 *
 * @details Processes the draw commands and submits them to the command buffer
 * for drawing.
 *
 * @param butter The butter context.
 * @param cmds The draw commands to submit.
 * @param count The amount of draw commands.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c cmds must be a valid pointer to an array of draw commands.
 * - @c count must be greater than 0 and match the size of the draw commands
 * array ptr.
 */
void butter_submit_draws(butter_t *butter, const butter_draw_cmd_t *cmds,
                         u32 count);

//
//
//

/**
 * @brief Allocate vertices from the shared vertex/index buffer for drawing.
 *
 * @param butter The butter context.
 * @param vertex_count The amount of vertices to allocate.
 * @param stride The stride of each vertex.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c vertex_count must be greater than 0.
 * - @c stride must be greater than 0.
 *
 * @return The allocation handle for the vertices.
 */
butter_allocation_t butter_alloc_vertices(butter_t *butter, u32 vertex_count,
                                          u32 stride);

//
//
//

/**
 * @brief Allocate indices from the shared vertex/index buffer for drawing.
 *
 * @details, an extension of @ref butter_alloc_vertices, but the stride is
 * determined by the index type.
 *
 * @param butter The butter context.
 * @param index_count The amount of indices to allocate.
 * @param index_type The index type.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c index_count must be greater than 0.
 * - @c index_type must be a valid index type.
 *
 * @return The allocation handle for the indices.
 */
butter_allocation_t butter_alloc_indices(butter_t *butter, u32 index_count,
                                         vk_index_type_t index_type);

#endif // !BUTTER_GRAPHICS_DRAW_H

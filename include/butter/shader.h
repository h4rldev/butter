#ifndef BUTTER_SHADER_H
#define BUTTER_SHADER_H

/***********************************/

#include <htils/basictypes.h>

#include <butter/internal/types.h>
#include <butter/types.h>

/***********************************/

/**
 * @brief Get a registered shader by name.
 * @details Queries the context's shader registry for a shader with the given
 * name.
 *
 * @param butter The butter context.
 * @param name The name of the shader to get.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c name must be a valid C-string.
 *
 * @return The shader with the given name, or NULL if no shader with the given
 * name exists.
 */
butter_shader_t *butter_shader_get(butter_t *butter, const cstr *name);

//
//
//

/**
 * @brief Load a shader from a file.
 * @details Reads a SPIR-V shader file and registers it under @c name
 * (deduplicated), and returns the registered shader.
 *
 * @param butter The butter context.
 * @param arena The arena to load shaders into.
 * @param name The name to register the shader under.
 * @param path The path to the SPIR-V file.
 * @param stage The shader stage.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c arena must be a valid arena.
 * - @c name must be a valid C-string.
 * - @c path must be a valid C-string, that points to a valid file.
 * - @c stage must be a valid shader stage, see @ref butter_shader_stage_t for
 * more info.
 *
 * @return The registered shader, or null on error.
 */
butter_shader_t *butter_shader_load_file(butter_t *butter, arena_t *arena,
                                         const cstr *name, const cstr *path,
                                         butter_shader_stage_t stage);

//
//
//

/**
 * @brief Load a shader from memory.
 * @details Reads a SPIR-V shader from memory and registers it under @c name
 * (deduplicated). Use with embedded shaders, e.g 'xxd -i shader.spv'.
 *
 * @param butter The butter context.
 * @param arena The arena to load shaders into.
 * @param name The name to register the shader under.
 * @param stage The shader stage.
 * @param code The SPIR-V shader code.
 * @param code_size The size of the shader code.
 * @param entry_point The entry point of the shader.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c arena must be a valid arena.
 * - @c name must be a valid C-string.
 * - @c stage must be a valid shader stage, see @ref butter_shader_stage_t for
 * more info.
 * - @c code must be a valid pointer to @c code_size bytes.
 * - @c code_size must be greater than 0.
 * - @c entry_point must be a valid C-string.
 *
 * @return The registered shader, or null on error.
 */
butter_shader_t *butter_shader_from_memory(butter_t *butter, arena_t *arena,
                                           const cstr *name,
                                           butter_shader_stage_t stage,
                                           const void *code, u64 code_size,
                                           const cstr *entry_point);

#endif // !BUTTER_SHADER_H

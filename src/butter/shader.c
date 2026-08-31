/*******************************/

#include <string.h>

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/file.h>
#include <htils/string.h>

#include <butter/internal/types.h>
#include <butter/log.h>
#include <butter/shader.h>
#include <butter/types.h>

/*******************************/

/**
 * @brief Register a shader.
 * @details Registers a shader under @c name with the provided attributes.
 *
 * @param butter The butter context.
 * @param arena The arena to allocate the shader from.
 * @param name The name to register the shader under.
 * @param stage The shader stage.
 * @param code The shader code.
 * @param code_size The size of the shader code.
 * @param entry_point The entry point of the shader.
 *
 * @pre
 * - @c butter must be a valid butter context.
 * - @c arena must be a valid arena.
 * - @c name must be a valid C-string.
 * - @c stage must be a valid shader stage, see @ref butter_shader_stage_t for
 * more info.
 * - @c code must be a valid pointer for a region of @c code_size bytes.
 * - @c code_size must be greater than 0.
 * - @c entry_point must be a valid C-string.
 *
 * @return The registered shader, or null on error.
 */
static butter_shader_t *butter_shader_register(butter_t *butter, arena_t *arena,
                                               const cstr *name,
                                               butter_shader_stage_t stage,
                                               const void *code, u64 code_size,
                                               const cstr *entry_point) {
  butter_shader_t *existing = butter_shader_get(butter, name);
  if (existing)
    return existing;

  if (butter->shader_registry->count == butter->shader_registry->capacity) {
    u32 new_capacity = butter->shader_registry->capacity
                           ? butter->shader_registry->capacity * 2
                           : 8;

    struct butter_shader *next =
        arena_alloc_zeroed(arena, struct butter_shader, new_capacity);
    memcpy(next, butter->shader_registry->shaders,
           butter->shader_registry->count * sizeof(struct butter_shader));
    butter->shader_registry->shaders = next;
    butter->shader_registry->capacity = new_capacity;
  }

  struct butter_shader *shader =
      &butter->shader_registry->shaders[butter->shader_registry->count++];
  shader->name = name;
  shader->stage = stage;
  shader->code = code;
  shader->code_size = code_size;
  shader->entry_point = entry_point ? entry_point : "main";
  shader->spec = null;
  return shader;
}

//
//
//

butter_shader_t *butter_shader_get(butter_t *butter, const cstr *name) {
  if (!butter || !name || !butter->shader_registry) {
    butter_log_error("Invalid arguments");
    return null;
  }

  for (u32 i = 0; i < butter->shader_registry->count; i++)
    if (memcmp(butter->shader_registry->shaders[i].name, name,
               strlen(name) + 1) == 0)
      return &butter->shader_registry->shaders[i];

  return null;
}

butter_shader_t *butter_shader_load_file(butter_t *butter, arena_t *arena,
                                         const cstr *name, const cstr *path,
                                         butter_shader_stage_t stage) {
  string *path_str = string_from_cstr(arena, path);
  string *file = read_file(arena, path_str);
  if (!file) {
    butter_log_error("Failed to read file %s", path);
    return null;
  }

  return butter_shader_register(butter, arena, name, stage, file->base,
                                file->len, "main");
}

butter_shader_t *butter_shader_from_memory(butter_t *butter, arena_t *arena,
                                           const cstr *name,
                                           butter_shader_stage_t stage,
                                           const void *code, u64 code_size,
                                           const cstr *entry_point) {
  if (!code || code_size == 0) {
    butter_log_error("Invalid shader code");
    return null;
  }

  return butter_shader_register(butter, arena, name, stage, code, code_size,
                                entry_point);
}

#ifndef BUTTER_INTERNAL_CACHE_H
#define BUTTER_INTERNAL_CACHE_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/string.h>

/***********************************/

/**
 * @brief Reads the pipeline cache file.
 * @details Returns null on error or failure to read the file.
 *
 * @param arena The arena to allocate the string from.
 * @param path The path to the file.
 *
 * @pre
 * - @c arena must be a valid arena.
 * - @c path must be a valid C-string.
 *
 * @return The string, or null on error.
 */
string *butter_read_cache_file(arena_t *arena, const cstr *path);

//
//
//

/**
 * @brief Writes the pipeline cache file to @c path.
 * @details Returns false on error or failure to write the file.
 *
 * @param path The path to the file.
 * @param data The data to write.
 * @param size The size of the data.
 *
 * @pre
 * - @c path must be a valid C-string.
 * - @c data must be a valid pointer to @c size bytes.
 * - @c size must be greater than 0.
 *
 * @return true on success, false on error.
 */
b32 butter_write_cache_file(const cstr *path, const void *data, u64 size);

#endif // !BUTTER_INTERNAL_CACHE_H

/***********************************/

#include <htils/arena.h>
#include <htils/basictypes.h>
#include <htils/file.h>
#include <htils/string.h>

#include <butter/internal/cache.h>

/***********************************/

string *butter_read_cache_file(arena_t *arena, const cstr *path) {
  FILE *stream = fopen(path, "rb");
  if (!stream)
    return null;

  u64 size = file_size_stream(stream);
  if (size == 0) {
    fclose(stream);
    return null;
  }

  string *data = string_new(arena, size);
  if (fread(data->base, 1, size, stream) != size) {
    fclose(stream);
    return null;
  }

  fclose(stream);
  return data;
}

b32 butter_write_cache_file(const cstr *path, const void *data, u64 size) {
  FILE *stream = fopen(path, "wb");
  if (!stream)
    return false;

  b32 ok = fwrite(data, 1, size, stream) == size;
  fclose(stream);
  return ok;
}

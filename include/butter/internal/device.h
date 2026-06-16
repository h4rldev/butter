#ifndef BUTTER_INTERNAL_DEVICE_H
#define BUTTER_INTERNAL_DEVICE_H

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/internal/types.h>

b32 butter_select_physical_device(arena_t *arena, butter_context_t *context);
b32 butter_create_device(butter_context_t *context);

#endif // !BUTTER_INTERNAL_DEVICE_H

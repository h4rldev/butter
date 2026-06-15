#ifndef BUTTER_INSTANCE_H
#define BUTTER_INSTANCE_H

#include <htils/arena.h>
#include <htils/basictypes.h>

#include <butter/check.h>
#include <butter/types.h>

vk_instance_t butter_create_instance(arena_t *arena, const cstr *app_name,
                                     b32 validation, butter_backend_t backend);

#endif // !BUTTER_INSTANCE_H

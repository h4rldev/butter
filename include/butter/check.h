#ifndef BUTTER_CHECK_H
#define BUTTER_CHECK_H

#include <butter/types.h>
#include <htils/basictypes.h>

b32 butter_is_vulkan_available(void);

const cstr *const *
butter_get_required_instance_extensions(butter_backend_t backend, u32 *count);

#endif // !BUTTER_CHECK_H

#ifndef BUTTER_INTERNAL_CHECK_H
#define BUTTER_INTERNAL_CHECK_H

/***********************************/

#include <htils/basictypes.h>

/***********************************/

/**
 * @brief Check if Vulkan is available.
 * @details Checks if Vulkan is available using runtime linking, then checking
 * if some addresses are available.
 *
 * @return true if Vulkan is available, false otherwise.
 */
b32 butter_is_vulkan_available(void);

#endif // !BUTTER_INTERNAL_CHECK_H

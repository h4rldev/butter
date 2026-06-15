#ifndef BUTTER_PRESENT_H
#define BUTTER_PRESENT_H

#include <butter/types.h>
#include <htils/basictypes.h>
#include <vulkan/vulkan.h>

vk_result_t butter_acquire_next_image(butter_context_t *context,
                                      u32 *image_index);
vk_result_t butter_submit_and_present(butter_context_t *context,
                                      vk_command_buffer_t cmd, u32 image_index);
u32 butter_get_frame_index(butter_context_t *context);

#endif // !BUTTER_PRESENT_H

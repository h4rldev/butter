/***********************************/

#include <htils/basictypes.h>

#include <butter/log.h>
#include <butter/types.h>

/***********************************/

butter_sampler_desc_t butter_sampler_desc_linear_clamp(void) {
  return (butter_sampler_desc_t){
      .mag_filter = VK_FILTER_LINEAR,
      .min_filter = VK_FILTER_LINEAR,
      .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .max_anisotropy = 0.0f,
      .compare_enable = false,
      .compare_op = VK_COMPARE_OP_NEVER,
  };
}

butter_sampler_desc_t butter_sampler_desc_linear_repeat(void) {
  return (butter_sampler_desc_t){
      .mag_filter = VK_FILTER_LINEAR,
      .min_filter = VK_FILTER_LINEAR,
      .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .max_anisotropy = 0.0f,
      .compare_enable = false,
      .compare_op = VK_COMPARE_OP_NEVER,
  };
}

butter_sampler_desc_t butter_sampler_desc_nearest_clamp(void) {
  return (butter_sampler_desc_t){
      .mag_filter = VK_FILTER_NEAREST,
      .min_filter = VK_FILTER_NEAREST,
      .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .max_anisotropy = 0.0f,
      .compare_enable = false,
      .compare_op = VK_COMPARE_OP_NEVER,
  };
}

butter_sampler_desc_t butter_sampler_desc_anisotropic(f32 max_anisotropy) {
  return (butter_sampler_desc_t){
      .mag_filter = VK_FILTER_LINEAR,
      .min_filter = VK_FILTER_LINEAR,
      .mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .address_mode_u = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_v = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .address_mode_w = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .max_anisotropy = max_anisotropy,
      .compare_enable = false,
      .compare_op = VK_COMPARE_OP_NEVER,
  };
}

vk_sampler_t butter_create_sampler(butter_t *butter,
                                   const butter_sampler_desc_t *desc) {
  if (!butter || !desc) {
    butter_log_error("Invalid arguments");
    return VK_NULL_HANDLE;
  }

  vk_result_t res;

  vk_sampler_create_info_t sampler_info = {0};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = desc->mag_filter;
  sampler_info.minFilter = desc->min_filter;
  sampler_info.mipmapMode = desc->mipmap_mode;
  sampler_info.addressModeU = desc->address_mode_u;
  sampler_info.addressModeV = desc->address_mode_v;
  sampler_info.addressModeW = desc->address_mode_w;
  sampler_info.minLod = 0.0f;
  sampler_info.maxLod = VK_LOD_CLAMP_NONE;
  sampler_info.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  sampler_info.anisotropyEnable = desc->max_anisotropy > 1.0f;
  sampler_info.maxAnisotropy = desc->max_anisotropy;
  sampler_info.compareEnable = desc->compare_enable;
  sampler_info.compareOp = desc->compare_op;

  vk_sampler_t sampler;
  if ((res = vkCreateSampler(butter->device, &sampler_info, null, &sampler)) !=
      VK_SUCCESS) {
    butter_log_error("Could not create sampler: %d", res);
    return VK_NULL_HANDLE;
  }

  return sampler;
}

void butter_destroy_sampler(butter_t *butter, vk_sampler_t sampler) {
  if (!butter)
    return;

  if (sampler)
    vkDestroySampler(butter->device, sampler, null);
}

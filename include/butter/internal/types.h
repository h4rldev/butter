#ifndef BUTTER_INTERNAL_TYPES_H
#define BUTTER_INTERNAL_TYPES_H

#include <vulkan/vulkan_core.h>
#include <xcb/xcb.h>

#include <htils/arena.h>
#include <htils/atomic_types.h>
#include <htils/darray.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_xcb.h>

#include <threads.h>

/** Aliases */
typedef PFN_vkGetInstanceProcAddr pfn_vkGetInstanceProcAddr;
typedef PFN_vkEnumerateInstanceVersion pfn_vkEnumerateInstanceVersion;
typedef PFN_vkCreateInstance pfn_vkCreateInstance;

typedef VkInstance vk_instance_t;
typedef VkInstanceCreateInfo vk_instance_create_info_t;
typedef VkApplicationInfo vk_application_info_t;
typedef VkSurfaceKHR vk_surface_khr_t;
typedef VkXcbSurfaceCreateInfoKHR vk_xcb_surface_create_info_khr_t;
typedef VkWaylandSurfaceCreateInfoKHR vk_wayland_surface_create_info_khr_t;
typedef VkPhysicalDevice vk_physical_device_t;
typedef VkPhysicalDeviceProperties vk_physical_device_properties_t;
typedef VkPhysicalDeviceMemoryProperties vk_physical_device_memory_properties_t;

typedef VkSurfaceCapabilitiesKHR vk_surface_capabilities_khr_t;
typedef VkDevice vk_device_t;
typedef VkDeviceQueueCreateInfo vk_device_queue_create_info_t;
typedef VkDeviceCreateInfo vk_device_create_info_t;
typedef VkQueue vk_queue_t;
typedef VkBool32 vk_bool32_t;
typedef VkQueueFamilyProperties vk_queue_family_properties_t;
typedef VkSwapchainKHR vk_swapchain_khr_t;
typedef VkSwapchainCreateInfoKHR vk_swapchain_create_info_khr_t;
typedef VkImage vk_image_t;
typedef VkImageCreateInfo vk_image_create_info_t;
typedef VkImageView vk_image_view_t;
typedef VkImageViewCreateInfo vk_image_view_create_info_t;
typedef VkImageMemoryBarrier vk_image_memory_barrier_t;
typedef VkImageSubresourceLayers vk_image_subresource_layers_t;
typedef VkImageSubresourceRange vk_image_subresource_range_t;

typedef VkFilter vk_filter_t;

typedef VkAttachmentDescription vk_attachment_description_t;
typedef VkAttachmentReference vk_attachment_reference_t;
typedef VkSubpassDescription vk_subpass_description_t;
typedef VkSemaphore vk_semaphore_t;
typedef VkSemaphoreCreateInfo vk_semaphore_create_info_t;
typedef VkResult vk_result_t;
typedef VkFormat vk_format_t;
typedef VkExtent2D vk_extent2d_t;
typedef VkExtent3D vk_extent3d_t;
typedef VkOffset3D vk_offset3d_t;
typedef VkOffset2D vk_offset2d_t;
typedef VkRenderPass vk_render_pass_t;
typedef VkRenderPassCreateInfo vk_render_pass_create_info_t;
typedef VkRenderPassBeginInfo vk_render_pass_begin_info_t;
typedef VkFramebuffer vk_framebuffer_t;
typedef VkFramebufferCreateInfo vk_framebuffer_create_info_t;
typedef VkCommandBuffer vk_command_buffer_t;
typedef VkCommandPool vk_command_pool_t;
typedef VkCommandPoolCreateInfo vk_command_pool_create_info_t;
typedef VkCommandBufferAllocateInfo vk_command_buffer_allocate_info_t;
typedef VkCommandBufferBeginInfo vk_command_buffer_begin_info_t;
typedef VkClearValue vk_clear_value_t;
typedef VkDescriptorSet vk_descriptor_set_t;
typedef VkDescriptorSetLayout vk_descriptor_set_layout_t;
typedef VkDescriptorSetAllocateInfo vk_descriptor_set_allocate_info_t;
typedef VkDescriptorPool vk_descriptor_pool_t;
typedef VkDescriptorPoolSize vk_descriptor_pool_size_t;
typedef VkDescriptorPoolCreateInfo vk_descriptor_pool_create_info_t;
typedef VkWriteDescriptorSet vk_write_descriptor_set_t;
typedef VkDescriptorBufferInfo vk_descriptor_buffer_info_t;
typedef VkDescriptorImageInfo vk_descriptor_image_info_t;
typedef VkDescriptorSetLayoutBinding vk_descriptor_set_layout_binding_t;
typedef VkDescriptorSetLayoutCreateInfo vk_descriptor_set_layout_create_info_t;

typedef VkShaderStageFlagBits vk_shader_stage_flags_t;

typedef VkPipeline vk_pipeline_t;
typedef VkPipelineCache vk_pipeline_cache_t;
typedef VkPipelineLayout vk_pipeline_layout_t;
typedef VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info_t;
typedef VkPipelineShaderStageCreateInfo vk_pipeline_shader_stage_create_info_t;
typedef VkPolygonMode vk_polygon_mode_t;
typedef VkFrontFace vk_front_face_t;
typedef VkSpecializationInfo vk_specialization_info_t;

typedef VkPrimitiveTopology vk_primitive_topology_t;
typedef VkCullModeFlags vk_cull_mode_t;

typedef VkVertexInputAttributeDescription
    vk_vertex_input_attribute_description_t;
typedef VkVertexInputBindingDescription vk_vertex_input_binding_description_t;

typedef VkPipelineVertexInputStateCreateInfo
    vk_pipeline_vertex_input_state_create_info_t;
typedef VkPipelineViewportStateCreateInfo
    vk_pipeline_viewport_state_create_info_t;
typedef VkPipelineRasterizationStateCreateInfo
    vk_pipeline_rasterization_state_create_info_t;
typedef VkPipelineMultisampleStateCreateInfo
    vk_pipeline_multisample_state_create_info_t;
typedef VkPipelineColorBlendAttachmentState
    vk_pipeline_color_blend_attachment_state_t;
typedef VkPipelineColorBlendStateCreateInfo
    vk_pipeline_color_blend_state_create_info_t;
typedef VkPipelineDynamicStateCreateInfo
    vk_pipeline_dynamic_state_create_info_t;
typedef VkPipelineInputAssemblyStateCreateInfo
    vk_pipeline_input_assembly_state_create_info_t;
typedef VkPipelineDepthStencilStateCreateInfo
    vk_pipeline_depth_stencil_state_create_info_t;

typedef VkIndexType vk_index_type_t;

typedef VkDynamicState vk_dynamic_state_t;
typedef VkPresentModeKHR vk_present_mode_khr_t;

typedef VkGraphicsPipelineCreateInfo vk_graphics_pipeline_create_info_t;

typedef VkBuffer vk_buffer_t;
typedef VkBufferCreateInfo vk_buffer_create_info_t;
typedef VkBufferUsageFlags vk_buffer_usage_flags_t;
typedef VkBufferImageCopy vk_buffer_image_copy_t;

typedef VkDeviceMemory vk_device_memory_t;
typedef VkDeviceSize vk_device_size_t;

typedef VkMemoryRequirements vk_memory_requirements_t;
typedef VkMemoryPropertyFlags vk_memory_property_flags_t;
typedef VkMemoryAllocateInfo vk_memory_allocate_info_t;

typedef VkShaderModule vk_shader_module_t;
typedef VkShaderModuleCreateInfo vk_shader_module_create_info_t;

typedef VkSurfaceFormatKHR vk_surface_format_khr_t;
typedef VkPipelineStageFlags vk_pipeline_stage_flags_t;
typedef VkSubmitInfo vk_submit_info_t;
typedef VkPresentInfoKHR vk_present_info_khr_t;
typedef VkFence vk_fence_t;
typedef VkFenceCreateInfo vk_fence_create_info_t;

typedef VkViewport vk_viewport_t;
typedef VkRect2D vk_rect2d_t;

typedef VkSampler vk_sampler_t;
typedef VkSamplerCreateInfo vk_sampler_create_info_t;
typedef VkSamplerMipmapMode vk_sampler_mipmap_mode_t;
typedef VkSamplerAddressMode vk_sampler_address_mode_t;

typedef VkCompareOp vk_compare_op_t;

struct butter_frame {
  vk_command_buffer_t cmd;
  vk_framebuffer_t fb;
  vk_extent2d_t extent;
  vk_render_pass_t rp;
  u64 frame_start_ns;
  u32 image_index;
};

typedef void (*butter_draw_callback_t)(vk_command_buffer_t cmd,
                                       const struct butter_frame *frame,
                                       void *userdata);

typedef struct butter_context {
  vk_instance_t instance;
  vk_physical_device_t physical_device;
  vk_device_t device;
  vk_queue_t queue;
  u32 queue_family;

  vk_surface_khr_t surface;
  vk_swapchain_khr_t swapchain;
  vk_format_t format;
  vk_extent2d_t extent;
  u32 image_count;

  vk_image_t *images;
  vk_image_view_t *image_views;
  vk_framebuffer_t *framebuffers;
  vk_render_pass_t render_pass;
  vk_fence_t *in_flight_fences;
  vk_semaphore_t *image_available;
  vk_semaphore_t *rendering_finished;
  u32 frame_index;

  vk_command_pool_t cmd_pool;
  vk_command_buffer_t *cmds;
  vk_clear_value_t clear_color;

  thrd_t render_thread;
  mtx_t render_mutex;
  cnd_t frame_ready;
  cnd_t frame_done;

  atomic_b32 render_running;
  atomic_b32 frame_requested;
  atomic_b32 frame_completed;
  atomic_b32 vsync;
  atomic_f32 target_refresh_rate;

  arena_t *arena;
  arena_t *render_arena;

  butter_draw_callback_t draw_callback;
  void *draw_userdata;

  u32 pending_width;
  u32 pending_height;
  b32 resize_pending;
} butter_context_t;

#endif // !BUTTER_INTERNAL_TYPES_H

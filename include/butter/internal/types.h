#ifndef BUTTER_INTERNAL_TYPES_H
#define BUTTER_INTERNAL_TYPES_H

#include <xcb/xcb.h>

#include <htils/basictypes.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include <vulkan/vulkan_xcb.h>

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
typedef VkImageView vk_image_view_t;
typedef VkImageViewCreateInfo vk_image_view_create_info_t;
typedef VkAttachmentDescription vk_attachment_description_t;
typedef VkAttachmentReference vk_attachment_reference_t;
typedef VkSubpassDescription vk_subpass_description_t;
typedef VkSemaphore vk_semaphore_t;
typedef VkSemaphoreCreateInfo vk_semaphore_create_info_t;
typedef VkResult vk_result_t;
typedef VkFormat vk_format_t;
typedef VkExtent2D vk_extent2d_t;
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

typedef VkSurfaceFormatKHR vk_surface_format_khr_t;
typedef VkPipelineStageFlags vk_pipeline_stage_flags_t;
typedef VkSubmitInfo vk_submit_info_t;
typedef VkPresentInfoKHR vk_present_info_khr_t;
typedef VkFence vk_fence_t;
typedef VkFenceCreateInfo vk_fence_create_info_t;

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

  u32 pending_width;
  u32 pending_height;
  b32 resize_pending;
} butter_context_t;

#endif // !BUTTER_INTERNAL_TYPES_H

#ifndef VULKAN_TYPES_H
#define VULKAN_TYPES_H

#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

typedef struct VulkanApp {
  VkInstance vkInstance;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue queue;
  VkPipelineLayout pipelineLayout;
  VkFormat swapchainImageFormat;
  VkExtent2D swapchainExtent;
  VkSwapchainKHR swapchain;
  VkSurfaceKHR surface;
  VkRenderPass renderPass;
  VkPipeline pipeline;
  VkCommandPool commandPool;
  // pointers to arrays
  uint32_t imageCount;
  VkFramebuffer *framebuffers;
  VkImage *swapchainImages;
  VkImageView *swapchainImageViews;
  VkSemaphore *imageAvailableSemaphores;
  VkSemaphore *renderFinishedSemaphores;
  uint32_t framesInFlight;
  VkFence *inFlight;
  VkCommandBuffer *commandBuffers;
} VulkanApp;

#endif // !VULKAN_TYPES

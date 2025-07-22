#include "vulkan/vulkan_core.h"
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

void createInstance(VkInstance *instance) {
  uint32_t extCount = 0;
  const char **extensions = glfwGetRequiredInstanceExtensions(&extCount);
  uint32_t layersCount = 1;
  const char **layers = (const char *[]){"VK_LAYER_KHRONOS_validation"};
  VkApplicationInfo appInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               .apiVersion = VK_API_VERSION_1_4,
                               .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                               .applicationVersion = VK_MAKE_VERSION(1, 0, 0)};
  VkInstanceCreateInfo info = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                               .pApplicationInfo = &appInfo,
                               .enabledExtensionCount = extCount,
                               .ppEnabledExtensionNames = extensions,
                               .enabledLayerCount = layersCount,
                               .ppEnabledLayerNames = layers};
  VkResult result = vkCreateInstance(&info, NULL, instance);
  if (result != VK_SUCCESS) {
    printf("failed to create vulkan instance %d\n", result);
    exit(1);
  }
}

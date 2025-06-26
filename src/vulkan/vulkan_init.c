#include "vulkan/vulkan_core.h"
#include "vulkan_types.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

void createInstance(VulkanApp *app) {
  uint32_t extCount = 0;
  const char **extensions = glfwGetRequiredInstanceExtensions(&extCount);
  // TODO make validation as a toggle
  const char **validation = (const char *[]){"VK_LAYER_KHRONOS_validation"};
  VkApplicationInfo appInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               .apiVersion = VK_API_VERSION_1_3,
                               .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                               .applicationVersion = VK_MAKE_VERSION(1, 0, 0)};
  VkInstanceCreateInfo info = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                               .pApplicationInfo = &appInfo,
                               .enabledExtensionCount = extCount,
                               .ppEnabledExtensionNames = extensions,
                               .enabledLayerCount = 1,
                               .ppEnabledLayerNames = validation};
  VkResult result = vkCreateInstance(&info, NULL, &app->vkInstance);
  if (result != VK_SUCCESS) {
    printf("error creating vk instance\n");
    exit(1);
  }
}

VulkanApp initVulkanApp() {
  int error = 0;
  VulkanApp app = {};
  createInstance(&app);
  return app;
}

void destoryVulkanApp(VulkanApp *app) {
  vkDestroyInstance(app->vkInstance, NULL);
}

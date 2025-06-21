#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

GLFWwindow *window;

VkInstance vkInstance;

VkPhysicalDevice physicalDevice;

void createInstance() {
  uint32_t extCount = 0;
  const char **extensions = glfwGetRequiredInstanceExtensions(&extCount);
  uint32_t valCount = 1;
  const char **validation = (const char *[]){"VK_LAYER_KHRONOS_validation"};
  VkApplicationInfo appInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               .apiVersion = VK_API_VERSION_1_3,
                               .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                               .applicationVersion = VK_MAKE_VERSION(1, 0, 0)};
  VkInstanceCreateInfo info = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                               .pApplicationInfo = &appInfo,
                               .enabledExtensionCount = extCount,
                               .ppEnabledExtensionNames = extensions,
                               .enabledLayerCount = valCount,
                               .ppEnabledLayerNames = validation};
  VkResult result = vkCreateInstance(&info, NULL, &vkInstance);
  if (result != VK_SUCCESS) {
    fprintf(stderr, "failed to create vulkan instance\n");
    exit(1);
  }
}

void pickPhysicalDevice() {
  uint32_t deviceCount;
  vkEnumeratePhysicalDevices(vkInstance, &deviceCount, NULL);
  if (deviceCount < 1) {
    printf("no compatible devices present");
    exit(1);
  }
  VkPhysicalDevice devices[deviceCount];
  vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);
  // skipped queue introspection logic for simplicity
  physicalDevice = devices[0];
}

void initWindow() {
  if (!glfwInit()) {
    printf("failed to init GLFW\n");
    exit(1);
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(800, 600, "Learn Vulkan", NULL, NULL);
}

void initVulkan() {
  initWindow();
  createInstance();
  pickPhysicalDevice();
}

void mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
  glfwDestroyWindow(window);
  glfwTerminate();
}

void cleanUp() {}

int main() {
  initVulkan();
  mainLoop();
  cleanUp();
  return 0;
}

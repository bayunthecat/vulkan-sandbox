#include "vulkan/vk_platform.h"
#include "vulkan/vulkan_core.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

typedef struct {
  int graphicalFamily;
  int presentFamily;
} QueueFamilyIndicies;

VkInstance vkInstance;

VkPhysicalDevice physicalDecice;

VkDevice device;

VkQueue graphicsQueue;

VkQueue presentQueue;

GLFWwindow *window;

VkSurfaceKHR surface;

VkSwapchainKHR swapchain;

VkFormat swapchainImageFormat;

VkExtent2D swapchainExtent;

VkImage swapchainImages[8];

VkImageView swapchainImageViews[8];

uint32_t swapchainImageCount;

VkBuffer vertexBuffer;

VkDeviceMemory vertexBufferMemory;

VkShaderModule vertShaderModule;

VkShaderModule fragShaderModule;

VkPipelineLayout pipelineLayout;

VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                  VK_DYNAMIC_STATE_SCISSOR};

VkPipeline graphicsPipeline;

VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount =
        (uint32_t)(sizeof(dynamicStates) / sizeof(dynamicStates[0])),
    .pDynamicStates = dynamicStates,
};

typedef struct {
  float pos[2];
} Vertex;

Vertex triangleVertices[] = {{{0.0f, -0.5f}}, {{0.5f, 0.5f}}, {{-0.5f, 0.5f}}};

VkVertexInputBindingDescription getBindingDescription() {
  VkVertexInputBindingDescription desc = {.binding = 0,
                                          .stride = sizeof(Vertex),
                                          .inputRate =
                                              VK_VERTEX_INPUT_RATE_VERTEX};
  return desc;
}

VkVertexInputAttributeDescription getAttributeDescription() {
  VkVertexInputAttributeDescription attr = {.binding = 0,
                                            .location = 0,
                                            .format = VK_FORMAT_R32G32_SFLOAT,
                                            .offset = offsetof(Vertex, pos)};
  return attr;
}

void createVulkanInstance(VkInstance *instance) {
  uint32_t extCount = 0;
  const char **extensions = glfwGetRequiredInstanceExtensions(&extCount);
  printf("vulkan glfw extension count: %d\n", extCount);
  VkApplicationInfo appInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                               .apiVersion = VK_API_VERSION_1_3,
                               .pApplicationName = "Hello triangle",
                               .pEngineName = "None",
                               .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                               .applicationVersion = VK_MAKE_VERSION(1, 0, 0)};
  VkInstanceCreateInfo info = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                               .pApplicationInfo = &appInfo,
                               .enabledExtensionCount = extCount,
                               .ppEnabledExtensionNames = extensions};
  VkResult result = vkCreateInstance(&info, NULL, instance);
  if (result != VK_SUCCESS) {
    fprintf(stderr, "Failed to create vulkan instance");
    exit(1);
  }
}

void pickPhysicalDevice(VkSurfaceKHR surface) {
  uint32_t devCount = 0;
  vkEnumeratePhysicalDevices(vkInstance, &devCount, NULL);
  VkPhysicalDevice devices[devCount];
  if (devCount == 0) {
    fprintf(stderr, "No graphics devices present");
    exit(1);
  }
  vkEnumeratePhysicalDevices(vkInstance, &devCount, devices);
  physicalDecice = devices[0];
}

void createLogicalDevice() {
  float pQueuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = 0,
      .queueCount = 1,
      .pQueuePriorities = &pQueuePriority};
  VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeature = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
      .dynamicRendering = VK_TRUE};
  const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &dynamicRenderingFeature,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueCreateInfo,
      .enabledExtensionCount = 1,
      .ppEnabledExtensionNames = deviceExtensions,
  };
  if (vkCreateDevice(physicalDecice, &createInfo, NULL, &device) !=
      VK_SUCCESS) {
    fprintf(stderr, "Logical device creation failed");
    exit(1);
  }
  vkGetDeviceQueue(device, 0, 0, &graphicsQueue);
  presentQueue = graphicsQueue;
}

void createSurfaceAndSwapchain() {
  window = glfwCreateWindow(800, 600, "Hello triangle", NULL, NULL);
  if (!window) {
    fprintf(stderr, "window creation failed");
    exit(1);
  }
  VkResult surfaceResult =
      glfwCreateWindowSurface(vkInstance, window, NULL, &surface);
  if (surfaceResult != VK_SUCCESS) {
    fprintf(stderr, "failed to create window surface, error: %d",
            surfaceResult);
    exit(1);
  }
  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDecice, surface, &caps);
  VkSurfaceFormatKHR surfaceFormat = {.format = VK_FORMAT_B8G8R8A8_UNORM,
                                      .colorSpace =
                                          VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
  swapchainExtent = caps.currentExtent;
  VkSwapchainCreateInfoKHR swapchainInfo = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = surface,
      .minImageCount = caps.minImageCount + 1,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent = swapchainExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = caps.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .clipped = VK_TRUE};
  VkResult swapchainCreateResult =
      vkCreateSwapchainKHR(device, &swapchainInfo, NULL, &swapchain);
  if (swapchainCreateResult != VK_SUCCESS) {
    fprintf(stderr, "Swapchain creation failed. error code: %d",
            swapchainCreateResult);
    exit(1);
  }
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
  vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount,
                          swapchainImages);
  swapchainImageFormat = surfaceFormat.format;
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    VkImageViewCreateInfo viewInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchainImageFormat,
        .components = {VK_COMPONENT_SWIZZLE_IDENTITY},
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};
    VkResult createImageResult =
        vkCreateImageView(device, &viewInfo, NULL, &swapchainImageViews[i]);
    if (createImageResult != VK_SUCCESS) {
      fprintf(stderr, "Failed to create image %i, error code: %d\n", i,
              createImageResult);
      exit(1);
    }
    printf("Image %d created\n", i);
  }
}

void createVertexBuffer() {
  VkDeviceSize bufferSize = sizeof(triangleVertices);
  VkBufferCreateInfo bufferInfo = {.sType =
                                       VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                   .size = bufferSize,
                                   .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
  VkResult createBufferResult =
      vkCreateBuffer(device, &bufferInfo, NULL, &vertexBuffer);
  if (createBufferResult != VK_SUCCESS) {
    fprintf(stderr, "Failed to create vertex buffer, error code: %d\n",
            createBufferResult);
    exit(1);
  }
  VkMemoryRequirements memReq;
  vkGetBufferMemoryRequirements(device, vertexBuffer, &memReq);
  VkPhysicalDeviceMemoryProperties memProps;
  vkGetPhysicalDeviceMemoryProperties(physicalDecice, &memProps);
  uint32_t memoryTypeIndex = UINT32_MAX;
  for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
    if ((memReq.memoryTypeBits & (1 << i)) &&
        (memProps.memoryTypes[i].propertyFlags &
         (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) ==
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
      memoryTypeIndex = i;
      break;
    }
  }
  if (memoryTypeIndex == UINT32_MAX) {
    fprintf(stderr, "Failed to find suitable memory type\n");
    exit(1);
  }
  VkMemoryAllocateInfo allocInfo = {.sType =
                                        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                    .allocationSize = memReq.size,
                                    .memoryTypeIndex = memoryTypeIndex};
  VkResult allocRes =
      vkAllocateMemory(device, &allocInfo, NULL, &vertexBufferMemory);
  if (allocRes != VK_SUCCESS) {
    fprintf(stderr, "Faild to allocate verex buffer memory, error code %d\n",
            allocRes);
    exit(1);
  }
  vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
}

void uploadVertexData() {
  void *data;
  if (vkMapMemory(device, vertexBufferMemory, 0, sizeof(triangleVertices), 0,
                  &data) != VK_SUCCESS) {
    fprintf(stderr, "Failed to map memory");
    exit(1);
  }
  memcpy(data, triangleVertices, sizeof(triangleVertices));
  vkUnmapMemory(device, vertexBufferMemory);
}

VkShaderModule loadShaderModule(const char *filepath) {
  FILE *file = NULL;
  int ret = fopen_s(&file, filepath, "rb");
  if (ret != 0 || !file) {
    fprintf(stderr, "Failed to open shader: %s, ret: %d\n", filepath, ret);
    perror("");
    exit(1);
  }
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  uint32_t *code = malloc(size);
  fread(code, 1, size, file);
  fclose(file);

  VkShaderModuleCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = size,
      .pCode = code};
  VkShaderModule shaderModule;
  VkResult moduleResult =
      vkCreateShaderModule(device, &info, NULL, &shaderModule);
  if (moduleResult != VK_SUCCESS) {
    fprintf(stderr, "Failed to create shader module: %s, code: %d\n", filepath,
            moduleResult);
    exit(1);
  }
  free(code);
  return shaderModule;
}

void createShaders() {
  vertShaderModule = loadShaderModule("triangle.vert.spv");
  fragShaderModule = loadShaderModule("triangle.frag.spv");
}

void createPipelineLayout() {
  VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pSetLayouts = NULL,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = NULL};
  VkResult result =
      vkCreatePipelineLayout(device, &info, NULL, &pipelineLayout);
  if (result != VK_SUCCESS) {
    fprintf(stderr, "Filed to create pipeline, error code: %d", result);
    exit(1);
  }
}

void createGraphicsPipeline() { VkVertexInputBindingDescription bindingDesc; }

int main() {
  if (!glfwInit()) {
    printf("Failed to init GLFW\n");
    exit(1);
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  createVulkanInstance(&vkInstance);
  pickPhysicalDevice(NULL);
  createLogicalDevice();
  createSurfaceAndSwapchain();
  createVertexBuffer();
  createShaders();
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

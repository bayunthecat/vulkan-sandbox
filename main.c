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

VkDevice device;

VkQueue queue;

VkImage *swapchainImages;

VkImageView *swapchainImageViews;

VkPipelineLayout pipelineLayout;

uint32_t imageCount = 0;

VkFormat swapchainImageFormat;

VkExtent2D swapchainExtent;

VkSwapchainKHR swapchain;

VkSurfaceKHR surface;

VkRenderPass renderPass;

void createInstance() {
  printf("creating vulkan instance\n");
  uint32_t extCount = 0;
  const char **extensions = glfwGetRequiredInstanceExtensions(&extCount);
  printf("glfw required extensions:\n");
  uint32_t valCount = 1;
  for (uint32_t i = 0; i < extCount; i++) {
    printf("%s\n", extensions[i]);
  }
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
  printf("selecting physical device\n");
  uint32_t deviceCount;
  vkEnumeratePhysicalDevices(vkInstance, &deviceCount, NULL);
  if (deviceCount < 1) {
    printf("no compatible devices present\n");
    exit(1);
  }
  VkPhysicalDevice devices[deviceCount];
  vkEnumeratePhysicalDevices(vkInstance, &deviceCount, devices);
  // skipped queue introspection logic for simplicity
  physicalDevice = devices[0];
}

void createLogicalDevice() {
  printf("creating logical device\n");
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = 0,
      .queueCount = 1,
      .pQueuePriorities = &queuePriority};
  VkPhysicalDeviceFeatures features = {};
  const char **ext = (const char *[]){VK_KHR_SWAPCHAIN_EXTENSION_NAME};
  VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,

      .pQueueCreateInfos = &queueCreateInfo,
      .pEnabledFeatures = &features,
      .enabledExtensionCount = 1,
      .ppEnabledExtensionNames = ext};
  VkResult createDeviceResult =
      vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, &device);
  if (createDeviceResult != VK_SUCCESS) {
    printf("failed to create logical device, error code: %d\n",
           createDeviceResult);
  }
}

void getDeviceQueues() {
  printf("getting device queue\n");
  vkGetDeviceQueue(device, 0, 0, &queue);
}

void createSurface() {
  printf("creating surface\n");
  VkResult result = glfwCreateWindowSurface(vkInstance, window, NULL, &surface);
  if (result != VK_SUCCESS) {
    printf("failed to create window surface, error code: %d\n", result);
    exit(1);
  }
}

void createSwapchain() {
  printf("creating swapchain\n");
  VkSurfaceCapabilitiesKHR surfaceCaps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &surfaceCaps);
  VkExtent2D extent = {
      .width = 800,
      .height = 600,
  };
  uint32_t swapchainImageCount = surfaceCaps.minImageCount + 1;
  VkFormat imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
  VkSwapchainCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .imageFormat = imageFormat,
      .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
      .presentMode = VK_PRESENT_MODE_FIFO_KHR,
      .imageExtent = extent,
      .surface = surface,
      .minImageCount = swapchainImageCount,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = 0,
      .preTransform = surfaceCaps.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE,
  };
  VkResult result = vkCreateSwapchainKHR(device, &info, NULL, &swapchain);
  if (result != VK_SUCCESS) {
    printf("failed to create swapchain");
    exit(1);
  }
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL);
  if (imageCount == 0) {
    printf("no images in the swapchain");
    exit(1);
  }
  printf("swapchain images: %d\n", imageCount);
  swapchainImages = malloc(sizeof(VkImage) * imageCount);
  if (swapchainImages == NULL) {
    printf("malloc failed");
    exit(1);
  }
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages);
  swapchainImageFormat = imageFormat;
  swapchainExtent = extent;
}

void createImageViews() {
  printf("creating image views\n");
  swapchainImageViews = malloc(sizeof(VkImageView) * imageCount);
  if (swapchainImageViews == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  for (uint32_t i = 0; i < imageCount; i++) {
    VkImageViewCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = swapchainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchainImageFormat,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };
    VkResult result =
        vkCreateImageView(device, &info, NULL, &swapchainImageViews[i]);
    if (result != VK_SUCCESS) {
      printf("failed to create image view: %d\n", result);
      exit(1);
    }
  }
}

uint32_t *loadRaw(const char *filepath, size_t *size) {
  FILE *file = NULL;
  int ret = fopen_s(&file, filepath, "rb");
  if (ret != 0 || !file) {
    fprintf(stderr, "Failed to open shader: %s, ret: %d\n", filepath, ret);
    perror("");
    exit(1);
  }
  fseek(file, 0, SEEK_END);
  *size = ftell(file);
  rewind(file);
  uint32_t *code = malloc(*size);
  if (code == NULL) {
    printf("malloc failed\n");
  }
  fread(code, 1, *size, file);
  fclose(file);
  return code;
}

VkShaderModule createShaderModule(const char *filepath) {
  size_t size;
  uint32_t *code = loadRaw(filepath, &size);
  VkShaderModuleCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pCode = code,
      .codeSize = size,
  };
  VkShaderModule module;
  VkResult result = vkCreateShaderModule(device, &info, NULL, &module);
  if (result != VK_SUCCESS) {
    printf("failed to create shader module %s\n", filepath);
    exit(1);
  }
  free(code);
  return module;
}

void createGraphicsPipeline() {
  VkShaderModule triVert = createShaderModule("shaders/comp/tri.vert.spv");
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = triVert,
      .pName = "main",
  };
  VkShaderModule triFrag = createShaderModule("shaders/comp/tri.frag.spv");
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = triFrag,
      .pName = "main",
  };
  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo,
                                                    fragShaderStageInfo};
  VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamicStates,
  };
  VkGraphicsPipelineCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
  };
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = NULL,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = NULL,
  };
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };
  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)swapchainExtent.width,
      .height = (float)swapchainExtent.height,
      .minDepth = 0.0f,
      .maxDepth = 0.0f,
  };
  VkRect2D scissor = {
      .offset = {0, 0},
      .extent = swapchainExtent,
  };
  VkPipelineViewportStateCreateInfo viewportStateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
  };
  VkPipelineRasterizationStateCreateInfo rasterInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
  };
  VkPipelineMultisampleStateCreateInfo multisampleInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .sampleShadingEnable = VK_FALSE,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
      .blendEnable = VK_FALSE,
  };
  VkPipelineColorBlendStateCreateInfo colorBlending = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
  };
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
  };
  VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                                           &pipelineLayout);
  if (result != VK_SUCCESS) {
    printf("failed pipeline layout\n");
    exit(1);
  }
  vkDestroyShaderModule(device, triFrag, NULL);
  vkDestroyShaderModule(device, triVert, NULL);
}

void createRenderPass() {
  VkAttachmentDescription colorAttachment = {
      .format = swapchainImageFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT,
  };
  VkAttachmentReference colorAttachmentRef = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef,
  };
  VkRenderPassCreateInfo renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &colorAttachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
  };
  VkResult result =
      vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
  if (result != VK_SUCCESS) {
    printf("failed create render pass\n");
    exit(1);
  }
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
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  getDeviceQueues();
  createSwapchain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
}

void mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }
  glfwDestroyWindow(window);
  glfwTerminate();
}

void destroyImageViews() {
  for (uint32_t i = 0; i < imageCount; i++) {
    vkDestroyImageView(device, swapchainImageViews[i], NULL);
  }
}

void cleanUp() {
  vkDestroySurfaceKHR(vkInstance, surface, NULL);
  vkDestroySwapchainKHR(device, swapchain, NULL);
  destroyImageViews();
  vkDestroyPipelineLayout(device, pipelineLayout, NULL);
  vkDestroyDevice(device, NULL);
  vkDestroyInstance(vkInstance, NULL);
  free(swapchainImages);
  free(swapchainImageViews);
}

int main() {
  initVulkan();
  mainLoop();
  cleanUp();
  return 0;
}

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "cglm/cam.h"
#include "cglm/common.h"
#include "cglm/mat4.h"
#include "cglm/types.h"
#include "cglm/util.h"
#include "file_utils.h"
#include "stb_image.h"
#include "tinyobj_loader_c.h"
#include "vulkan/vulkan_core.h"
#include <GLFW/glfw3.h>
#include <cglm/affine-mat.h>
#include <cglm/affine.h>
#include <cglm/cglm.h>
#include <cglm/project.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vulkan/vulkan.h>

typedef struct {
  mat4 model;
  mat4 view;
  mat4 proj;
} UniformBufferObject;

typedef struct {
  vec3 vertex;
  vec3 color;
  vec2 texture;
} Vertex;

GLFWwindow *window;

VkInstance vkInstance;

VkPhysicalDevice physicalDevice;

VkDevice device;

VkQueue queue;

VkImage textureImage;

VkImageView textureImageView;

VkImage depthImage;

VkImageView depthImageView;

VkDeviceMemory depthImageMemory;

VkSampler textureSampler;

VkDeviceMemory textureMem;

VkImage *swapchainImages;

VkImageView *swapchainImageViews;

VkPipelineLayout pipelineLayout;

uint32_t imageCount = 0;

VkFormat swapchainImageFormat;

VkExtent2D swapchainExtent;

VkSwapchainKHR swapchain;

VkSurfaceKHR surface;

VkRenderPass renderPass;

VkPipeline pipeline;

VkFramebuffer *framebuffers;

VkCommandPool commandPool;

VkCommandBuffer *commandBuffers;

VkSemaphore *imageAvailableSemaphores;

VkSemaphore *renderFinishedSemaphores;

VkFence *inFlight;

VkBuffer vertexBuffer;

VkDeviceMemory vertexBufferMemory;

VkBuffer indexBuffer;

VkDeviceMemory indexBufferMemory;

VkBuffer *uniformBuffers;

VkDeviceMemory *uniformBuffersMemoryList;

void **uniformBuffersMapped;

VkDescriptorSetLayout descriptorLayout;

VkPipelineLayout pipelineLayout;

VkDescriptorPool descriptorPool;

VkDescriptorSet *descriptorSets;

const int MAX_FRAMES_IN_FLIGHT = 3;

uint32_t currentFrame = 0;

VkBuffer modelBuffer;

VkDeviceMemory modelBufferMemory;

vec3 *modelVerts;

uint32_t modelVertsNum;

vec2 *texCoords;

uint32_t numTexCoords;

Vertex *modelVertices;

int modelVerticesNum;

uint32_t *modelIndices;

uint32_t modelIndicesNum;

VkBuffer modelIndiciesBuffer;

VkDeviceMemory modelIndicesBufferMemory;

Vertex vertices[] = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
};

uint32_t indices[] = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

void createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboLayoutBinding = {
      .binding = 0,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
  };
  VkDescriptorSetLayoutBinding samplerLayoutBinding = {
      .binding = 1,
      .descriptorCount = 1,
      .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .pImmutableSamplers = NULL,
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
  };
  VkDescriptorSetLayoutBinding bindings[] = {
      uboLayoutBinding,
      samplerLayoutBinding,
  };
  VkDescriptorSetLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = 2,
      .pBindings = bindings,
  };
  if (vkCreateDescriptorSetLayout(device, &info, NULL, &descriptorLayout) !=
      VK_SUCCESS) {
    printf("Unable to create descriptor set layout\n");
    exit(1);
  };
}

VkVertexInputBindingDescription getVertexBindDesc() {
  VkVertexInputBindingDescription desc = {
      .binding = 0,
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      .stride = sizeof(Vertex),
  };
  return desc;
}

VkVertexInputAttributeDescription getVertexAttrDesc() {
  VkVertexInputAttributeDescription desc = {
      .binding = 0,
      .location = 0,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = offsetof(Vertex, vertex),
  };
  return desc;
}

VkVertexInputAttributeDescription getColorAttrDesc() {
  VkVertexInputAttributeDescription desc = {
      .binding = 0,
      .location = 1,
      .format = VK_FORMAT_R32G32B32_SFLOAT,
      .offset = offsetof(Vertex, color),
  };
  return desc;
}

VkVertexInputAttributeDescription getTextureAttrDesc() {
  VkVertexInputAttributeDescription desc = {
      .binding = 0,
      .location = 2,
      .format = VK_FORMAT_R32G32_SFLOAT,
      .offset = offsetof(Vertex, texture),
  };
  return desc;
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
  VkPhysicalDeviceMemoryProperties memProps = {};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
  for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
    if (typeFilter & (1 << i) &&
        (memProps.memoryTypes[i].propertyFlags & props) == props) {
      return i;
    }
  }
  printf("unable to find suitable memory\n");
  exit(1);
}

VkCommandBuffer beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandPool = commandPool,
      .commandBufferCount = 1,
  };
  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);
  VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,
  };
  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);
  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void transitionImageLayout(VkImage image, VkFormat format,
                           VkImageLayout oldLayout, VkImageLayout newLayout) {
  VkCommandBuffer cmdBuff = beginSingleTimeCommands();
  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = oldLayout,
      .newLayout = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      }};
  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else {
    printf("unsupported ransistion\n");
    exit(1);
  }
  vkCmdPipelineBarrier(cmdBuff, sourceStage, destinationStage, 0, 0, NULL, 0,
                       NULL, 1, &barrier);
  endSingleTimeCommands(cmdBuff);
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags props, VkBuffer *buffer,
                  VkDeviceMemory *memory) {
  VkBufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };
  if (vkCreateBuffer(device, &info, NULL, buffer) != VK_SUCCESS) {
    printf("error creating buffer\n");
    exit(1);
  }
  VkMemoryRequirements memReq;
  vkGetBufferMemoryRequirements(device, *buffer, &memReq);

  VkMemoryAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memReq.size,
      .memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, props),
  };
  if (vkAllocateMemory(device, &allocInfo, NULL, memory) != VK_SUCCESS) {
    printf("error allocating memory\n");
    exit(1);
  };
  vkBindBufferMemory(device, *buffer, *memory, 0);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkCommandBufferAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandPool = commandPool,
      .commandBufferCount = 1,
  };
  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  VkBufferCopy copyRegion = {
      .srcOffset = 0,
      .dstOffset = 0,
      .size = size,
  };
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  vkEndCommandBuffer(commandBuffer);
  VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,
  };
  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);
  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void createIndexBuffer() {
  VkDeviceSize size = sizeof(uint32_t) * 12;
  VkBuffer stage;
  VkDeviceMemory stageMemory;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &stage, &stageMemory);

  void *data;
  vkMapMemory(device, stageMemory, 0, size, 0, &data);
  memcpy(data, indices, size);
  vkUnmapMemory(device, stageMemory);

  createBuffer(
      size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

  copyBuffer(stage, indexBuffer, size);
  vkDestroyBuffer(device, stage, NULL);
  vkFreeMemory(device, stageMemory, NULL);
  printf("created index buffer\n");
}

void createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);
  uniformBuffers = malloc(sizeof(VkBuffer) * MAX_FRAMES_IN_FLIGHT);
  if (uniformBuffers == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  uniformBuffersMemoryList =
      malloc(sizeof(VkDeviceMemory) * MAX_FRAMES_IN_FLIGHT);
  if (uniformBuffersMemoryList == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  uniformBuffersMapped = malloc(sizeof(void *) * MAX_FRAMES_IN_FLIGHT);
  if (uniformBuffersMapped == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &uniformBuffers[i], &uniformBuffersMemoryList[i]);
    vkMapMemory(device, uniformBuffersMemoryList[i], 0, bufferSize, 0,
                &uniformBuffersMapped[i]);
  }
}

void createDescriptorPool() {
  VkDescriptorPoolSize poolSize = {
      .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = MAX_FRAMES_IN_FLIGHT,
  };
  VkDescriptorPoolSize samplerPoolSize = {
      .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      .descriptorCount = MAX_FRAMES_IN_FLIGHT,
  };
  VkDescriptorPoolSize poolSizes[] = {
      poolSize,
      samplerPoolSize,
  };
  VkDescriptorPoolCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .poolSizeCount = 2,
      .pPoolSizes = poolSizes,
      .maxSets = MAX_FRAMES_IN_FLIGHT,
  };
  if (vkCreateDescriptorPool(device, &info, NULL, &descriptorPool) !=
      VK_SUCCESS) {
    printf("failed create descriptor pool\n");
    exit(1);
  };
}

void createDescriptorSets() {
  VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    layouts[i] = descriptorLayout;
  }
  descriptorSets = malloc(sizeof(VkDescriptorSet) * MAX_FRAMES_IN_FLIGHT);
  if (descriptorSets == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  VkDescriptorSetAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = descriptorPool,
      .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
      .pSetLayouts = layouts,
  };
  if (vkAllocateDescriptorSets(device, &info, descriptorSets) != VK_SUCCESS) {
    printf("Unable to allocate descriptor sets\n");
    exit(1);
  }
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo = {
        .buffer = uniformBuffers[i],
        .offset = 0,
        .range = sizeof(UniformBufferObject),
    };
    VkWriteDescriptorSet bufferWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSets[i],
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .pBufferInfo = &bufferInfo,
    };
    VkDescriptorImageInfo imageInfo = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = textureImageView,
        .sampler = textureSampler,
    };
    VkWriteDescriptorSet samplerWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptorSets[i],
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .pImageInfo = &imageInfo};

    VkWriteDescriptorSet writes[] = {bufferWrite, samplerWrite};
    vkUpdateDescriptorSets(device, 2, writes, 0, NULL);
  }
}

void createImage(uint32_t width, uint32_t height, VkFormat format,
                 VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags props, VkImage *image,
                 VkDeviceMemory *imageMemory) {
  VkImageCreateInfo imageInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent =
          {
              .width = width,
              .height = height,
              .depth = 1,
          },
      .mipLevels = 1,
      .arrayLayers = 1,
      .format = format,
      .tiling = tiling,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .samples = VK_SAMPLE_COUNT_1_BIT,
  };
  if (vkCreateImage(device, &imageInfo, NULL, image) != VK_SUCCESS) {
    printf("image creation failed\n");
    exit(1);
  }
  VkMemoryRequirements memReq;
  vkGetImageMemoryRequirements(device, *image, &memReq);

  VkMemoryAllocateInfo allocInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memReq.size,
      .memoryTypeIndex = findMemoryType(memReq.memoryTypeBits,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
  if (vkAllocateMemory(device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
    printf("failed to allocate image memory\n");
    exit(1);
  }
  vkBindImageMemory(device, *image, *imageMemory, 0);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                       uint32_t height) {
  VkCommandBuffer cmdBuff = beginSingleTimeCommands();
  VkBufferImageCopy region = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageOffset = {0, 0, 0},
      .imageExtent = {width, height, 1},
      .imageSubresource =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .mipLevel = 0,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  vkCmdCopyBufferToImage(cmdBuff, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
  endSingleTimeCommands(cmdBuff);
}

void createTextureImage() {
  int texWidth, texHeight, texChannels;
  stbi_uc *pixels = stbi_load("assets/viking_room.png", &texWidth, &texHeight,
                              &texChannels, STBI_rgb_alpha);
  VkDeviceSize dSize = texWidth * texHeight * 4;
  VkBuffer stage;
  VkDeviceMemory stageMem;
  createBuffer(dSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
               &stage, &stageMem);
  void *data;
  vkMapMemory(device, stageMem, 0, dSize, 0, &data);
  memcpy(data, pixels, dSize);
  vkUnmapMemory(device, stageMem);
  free(pixels);

  createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB,
              VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
              &textureImage, &textureMem);
  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(stage, textureImage, texWidth, texHeight);
  transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void createTextureImageView() {
  VkImageViewCreateInfo viewInfo = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = textureImage,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_SRGB,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  if (vkCreateImageView(device, &viewInfo, NULL, &textureImageView) !=
      VK_SUCCESS) {
    printf("error creating texture image view\n");
    exit(1);
  }
}

void createTextureSampler() {
  VkPhysicalDeviceProperties props = {};
  vkGetPhysicalDeviceProperties(physicalDevice, &props);
  VkSamplerCreateInfo samplerInfo = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .anisotropyEnable = VK_TRUE,
      .maxAnisotropy = props.limits.maxSamplerAnisotropy,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
  };
  if (vkCreateSampler(device, &samplerInfo, NULL, &textureSampler) !=
      VK_SUCCESS) {
    printf("error creating sampler\n");
    exit(1);
  }
}

void createVertexBuffer() {
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;
  VkDeviceSize bufferSize = sizeof(Vertex) * 8;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &stagingBuffer, &stagingMemory);

  void *data;
  vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices, bufferSize);
  vkUnmapMemory(device, stagingMemory);
  createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);
  copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
  vkDestroyBuffer(device, stagingBuffer, NULL);
  vkFreeMemory(device, stagingMemory, NULL);
}

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
  VkPhysicalDeviceFeatures features = {
      .samplerAnisotropy = VK_TRUE,
  };
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
      .presentMode = VK_PRESENT_MODE_MAILBOX_KHR,
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
    printf("failed to create swapchain\n");
    exit(1);
  }
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL);
  if (imageCount == 0) {
    printf("no images in the swapchain\n");
    exit(1);
  }
  printf("swapchain images: %d\n", imageCount);
  swapchainImages = malloc(sizeof(VkImage) * imageCount);
  if (swapchainImages == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages);
  swapchainImageFormat = imageFormat;
  swapchainExtent = extent;
}

VkImageView createImageView(VkImage image, VkFormat format,
                            VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange =
          {
              .aspectMask = aspectFlags,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  VkImageView imageView;
  if (vkCreateImageView(device, &info, NULL, &imageView) != VK_SUCCESS) {
    printf("failed to create image view\n");
    exit(1);
  }
  return imageView;
}

void createImageViews() {
  printf("creating image views\n");
  swapchainImageViews = malloc(sizeof(VkImageView) * imageCount);
  if (swapchainImageViews == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  for (uint32_t i = 0; i < imageCount; i++) {
    swapchainImageViews[i] = createImageView(
        swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
  }
}

VkShaderModule createShaderModule(const char *filepath) {
  size_t size;
  uint32_t *code = load(filepath, &size);
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
  VkVertexInputAttributeDescription attr[] = {
      getVertexAttrDesc(),
      getColorAttrDesc(),
      getTextureAttrDesc(),
  };
  VkVertexInputBindingDescription binds[] = {
      getVertexBindDesc(),
  };
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexAttributeDescriptionCount = 3,
      .pVertexAttributeDescriptions = attr,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = binds,
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
      .maxDepth = 1.0f,
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
      .lineWidth = 1.0f,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
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
      .logicOp = VK_LOGIC_OP_COPY,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
      .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
  };
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
  };
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pushConstantRangeCount = 0,
      .pSetLayouts = &descriptorLayout,
  };
  VkResult result = vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL,
                                           &pipelineLayout);
  if (result != VK_SUCCESS) {
    printf("failed pipeline layout\n");
    exit(1);
  }
  VkGraphicsPipelineCreateInfo pipelineInfo = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = shaderStages,
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssemblyInfo,
      .pViewportState = &viewportStateInfo,
      .pRasterizationState = &rasterInfo,
      .pMultisampleState = &multisampleInfo,
      .pColorBlendState = &colorBlending,
      .pDynamicState = &dynamicStateInfo,
      .layout = pipelineLayout,
      .renderPass = renderPass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .pDepthStencilState = &depthStencilInfo,
  };
  VkResult pipelineResult = vkCreateGraphicsPipelines(
      device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pipeline);
  if (pipelineResult != VK_SUCCESS) {
    printf("failed to create pipeline\n");
    exit(1);
  }
  vkDestroyShaderModule(device, triFrag, NULL);
  vkDestroyShaderModule(device, triVert, NULL);
}

void createRenderPass() {
  VkAttachmentDescription colorAttachment = {
      .format = swapchainImageFormat,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };
  VkAttachmentDescription depthAttachment = {
      .format = VK_FORMAT_D32_SFLOAT,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };
  VkAttachmentReference colorAttachmentRef = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  VkAttachmentReference depthAttachmentRef = {
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };
  VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentRef,
      .pDepthStencilAttachment = &depthAttachmentRef,
  };
  VkSubpassDependency dependency = {
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .srcAccessMask = 0,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                       VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
  };
  VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};
  VkRenderPassCreateInfo renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 2,
      .pAttachments = attachments,
      .subpassCount = 1,
      .pSubpasses = &subpass,
      .dependencyCount = 1,
      .pDependencies = &dependency,

  };
  VkResult result =
      vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
  if (result != VK_SUCCESS) {
    printf("failed create render pass\n");
    exit(1);
  }
}

void createFramebuffers() {
  framebuffers = malloc(sizeof(VkFramebuffer) * imageCount);
  if (framebuffers == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  for (uint32_t i = 0; i < imageCount; i++) {
    VkImageView attachments[] = {
        swapchainImageViews[i],
        depthImageView,
    };
    VkFramebufferCreateInfo framebufferInfo = {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = 2,
        .pAttachments = attachments,
        .width = swapchainExtent.width,
        .height = swapchainExtent.height,
        .layers = 1,
    };
    VkResult result =
        vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]);
    if (result != VK_SUCCESS) {
      printf("failed to create framebuffer");
      exit(1);
    }
  }
}

void createCommandPool() {
  VkCommandPoolCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = 0,
  };
  VkResult result = vkCreateCommandPool(device, &info, NULL, &commandPool);
  if (result != VK_SUCCESS) {
    printf("failed to create command pool");
    exit(1);
  }
}

void createCommandBuffers() {
  commandBuffers = malloc(MAX_FRAMES_IN_FLIGHT * sizeof(VkCommandBuffer));
  if (commandBuffers == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  VkCommandBufferAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = MAX_FRAMES_IN_FLIGHT,
  };
  VkResult result = vkAllocateCommandBuffers(device, &info, commandBuffers);
  if (result != VK_SUCCESS) {
    printf("failed to allocate buffers\n");
    exit(1);
  }
}

void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
  VkCommandBufferBeginInfo begingInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  VkResult result = vkBeginCommandBuffer(commandBuffer, &begingInfo);
  if (result != VK_SUCCESS) {
    printf("failed go begin command buffer");
    exit(1);
  }
  VkClearValue clearColor = {
      .color = {{0.0f, 0.0f, 0.0f, 1.0f}},
  };
  VkClearValue clearDepth = {
      .depthStencil = {1.0f, 0},
  };
  VkClearValue clears[] = {clearColor, clearDepth};
  VkRenderPassBeginInfo renderPassInfo = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = renderPass,
      .framebuffer = framebuffers[imageIndex],
      .renderArea.offset = {0, 0},
      .renderArea.extent = swapchainExtent,
      .pClearValues = clears,
      .clearValueCount = 2,
  };
  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  VkViewport viewport = {
      .x = 0.0f,
      .y = 0.0f,
      .width = swapchainExtent.width,
      .height = swapchainExtent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  VkRect2D scissor = {
      .offset = {0, 0},
      .extent = swapchainExtent,
  };
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, &modelBuffer, offsets);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipelineLayout, 0, 1, &descriptorSets[currentFrame],
                          0, NULL);
  vkCmdDraw(commandBuffer, modelVerticesNum, 2, 0, 0);
  vkCmdEndRenderPass(commandBuffer);
  VkResult endBufferResult = vkEndCommandBuffer(commandBuffer);
  if (endBufferResult != VK_SUCCESS) {
    printf("end buffer failed\n");
    exit(1);
  }
}

void createSyncObjects() {
  imageAvailableSemaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
  if (imageAvailableSemaphores == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
  if (renderFinishedSemaphores == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  inFlight = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
  if (inFlight == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  VkSemaphoreCreateInfo semaphoreInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  VkFenceCreateInfo fenceInfo = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    printf("creating sync objects for index: %d\n", i);
    if (vkCreateSemaphore(device, &semaphoreInfo, NULL,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS) {
      printf("failed to create semaphore\n");
      exit(1);
    }
    if (vkCreateSemaphore(device, &semaphoreInfo, NULL,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS) {
      printf("failed to create semaphore");
      exit(1);
    }
    if (vkCreateFence(device, &fenceInfo, NULL, &inFlight[i]) != VK_SUCCESS) {
      printf("failed to create fence");
      exit(1);
    }
  }
}

clock_t start = 0;

void updateUniformBuffer(uint32_t currentImage) {
  if (start == 0) {
    start = clock();
  }
  clock_t currentTime = clock();
  float time = (float)(currentTime - start) / CLOCKS_PER_SEC;
  UniformBufferObject ubo = {
      .model = GLM_MAT4_IDENTITY_INIT,
      .view = GLM_MAT4_IDENTITY_INIT,
      .proj = GLM_MAT4_IDENTITY_INIT,
  };
  glm_rotate(ubo.model, time * glm_rad(90.0f), (vec3){0.0f, 1.0f, 0.0f});
  vec3 eye = {0.0f, 10.0f, 0.0f};
  vec3 center = {0.0f, 0.0f, 0.0f};
  vec3 up = {0.0f, 0.0f, -1.0f};
  glm_lookat(eye, center, up, ubo.view);
  glm_perspective(glm_rad(100.0f),
                  swapchainExtent.width / (float)swapchainExtent.height, 0.1f,
                  10.0f, ubo.proj);
  ubo.proj[1][1] *= -1;
  memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void drawFrame() {
  vkWaitForFences(device, 1, &inFlight[currentFrame], VK_TRUE, UINT64_MAX);

  uint32_t imageIndex;
  vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                        imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE,
                        &imageIndex);
  updateUniformBuffer(currentFrame);
  vkResetFences(device, 1, &inFlight[currentFrame]);
  vkResetCommandBuffer(commandBuffers[currentFrame], 0);
  recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submitInfo = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &imageAvailableSemaphores[currentFrame],
      .pWaitDstStageMask = waitStages,
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffers[currentFrame],
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &renderFinishedSemaphores[currentFrame],
  };
  if (vkQueueSubmit(queue, 1, &submitInfo, inFlight[currentFrame]) !=
      VK_SUCCESS) {
    printf("queue submit failed\n");
    exit(1);
  }
  VkPresentInfoKHR presentInfo = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &renderFinishedSemaphores[currentFrame],
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = &imageIndex,
  };
  VkResult result = vkQueuePresentKHR(queue, &presentInfo);
  if (result != VK_SUCCESS) {
    printf("present failed\n");
    exit(1);
  }
  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void createDepthResources() {
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
  createImage(
      swapchainExtent.width, swapchainExtent.height, depthFormat,
      VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthImage, &depthImageMemory);
  depthImageView =
      createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
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

void loadFile(void *ctx, const char *filename, const int isMtl,
              const char *objFilename, char **data, size_t *len) {
  if (isMtl == 1) {
    return;
  }
  *data = load(filename, len);
  printf("load file len: %lld\n", *len);
}

void loadModel(const char *filename, Vertex **vertices, int *numVertices) {
  tinyobj_attrib_t attrib;
  tinyobj_shape_t *shapes;
  tinyobj_material_t *materials;
  size_t numShapes;
  size_t numMaterials;
  int result =
      tinyobj_parse_obj(&attrib, &shapes, &numShapes, &materials, &numMaterials,
                        filename, loadFile, NULL, TINYOBJ_FLAG_TRIANGULATE);
  if (result != TINYOBJ_SUCCESS) {
    printf("Failed to load model, error: %d\n", result);
    exit(1);
  }
  vec3 verts[attrib.num_vertices];
  for (uint32_t i = 0; i < attrib.num_vertices; i++) {
    verts[i][0] = attrib.vertices[i * 3 + 0];
    verts[i][1] = attrib.vertices[i * 3 + 1];
    verts[i][2] = attrib.vertices[i * 3 + 2];
  }
  vec2 texCoords[attrib.num_texcoords];
  for (uint32_t i = 0; i < attrib.num_texcoords; i++) {
    texCoords[i][0] = attrib.texcoords[i * 2 + 0];
    texCoords[i][1] = 1.0f - attrib.texcoords[i * 2 + 1];
  }

  Vertex *v = malloc(sizeof(Vertex) * attrib.num_faces);
  if (v == NULL) {
    printf("malloc failed\n");
    exit(1);
  }
  *numVertices = attrib.num_faces;
  int r = 0;
  int count = 0;
  for (uint32_t i = 0; i < attrib.num_face_num_verts; i++) {
    for (uint32_t j = 0; j < attrib.face_num_verts[r]; j++) {
      uint32_t faceIdx = (i * 3) + j;
      tinyobj_vertex_index_t f = attrib.faces[faceIdx];
      memcpy(&v[count].vertex, &verts[f.v_idx], sizeof(vec3));
      memcpy(&v[count].texture, &texCoords[f.vt_idx], sizeof(vec2));
      count++;
    }
    r++;
  }
  *vertices = v;
}

void createModelBuffer() {
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;
  VkDeviceSize bufferSize = sizeof(Vertex) * modelVerticesNum;
  createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               &stagingBuffer, &stagingMemory);

  void *data;
  vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &data);
  memcpy(data, modelVertices, bufferSize);
  vkUnmapMemory(device, stagingMemory);
  createBuffer(
      bufferSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &modelBuffer, &modelBufferMemory);
  copyBuffer(stagingBuffer, modelBuffer, bufferSize);
  vkDestroyBuffer(device, stagingBuffer, NULL);
  vkFreeMemory(device, stagingMemory, NULL);
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
  createDescriptorSetLayout();
  createGraphicsPipeline();
  createCommandPool();
  createDepthResources();
  createFramebuffers();
  createTextureImage();
  createTextureImageView();
  createTextureSampler();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createCommandBuffers();
  createSyncObjects();
  createVertexBuffer();
  loadModel("assets/branch_t.obj", &modelVertices, &modelVerticesNum);
  createModelBuffer();
  createIndexBuffer();
}

void mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }
  vkDeviceWaitIdle(device);
}

void destroyImageViews() {
  for (uint32_t i = 0; i < imageCount; i++) {
    vkDestroyImageView(device, swapchainImageViews[i], NULL);
  }
}

void destroyFramebuffers() {
  for (uint32_t i = 0; i < imageCount; i++) {
    vkDestroyFramebuffer(device, framebuffers[i], NULL);
  }
}

void destroySyncObjects() {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
    vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
    vkDestroyFence(device, inFlight[i], NULL);
  }
  free(imageAvailableSemaphores);
  free(renderFinishedSemaphores);
  free(inFlight);
}

void destroyUniformBuffers() {
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroyBuffer(device, uniformBuffers[i], NULL);
    vkFreeMemory(device, uniformBuffersMemoryList[i], NULL);
    free(uniformBuffersMapped[i]);
  }
}

void cleanUp() {
  glfwDestroyWindow(window);
  glfwTerminate();
  vkDestroySwapchainKHR(device, swapchain, NULL);
  vkDestroySurfaceKHR(vkInstance, surface, NULL);
  destroyImageViews();
  destroyFramebuffers();
  vkDestroyPipelineLayout(device, pipelineLayout, NULL);
  vkDestroyPipeline(device, pipeline, NULL);
  vkDestroyRenderPass(device, renderPass, NULL);
  vkDestroyCommandPool(device, commandPool, NULL);
  vkDestroyBuffer(device, vertexBuffer, NULL);
  destroyUniformBuffers();
  vkFreeMemory(device, vertexBufferMemory, NULL);
  destroySyncObjects();
  vkDestroyDescriptorSetLayout(device, descriptorLayout, NULL);
  vkDestroyDevice(device, NULL);
  vkDestroyInstance(vkInstance, NULL);
  free(swapchainImages);
  free(swapchainImageViews);
  free(framebuffers);
}

int main() {
  initVulkan();
  mainLoop();
  cleanUp();
  return 0;
}

cmake_minimum_required(VERSION 3.10)
project(VulkanLearn)

set(CMAKE_C_STANDARD 99)

find_package(Vulkan REQUIRED)

add_executable(main main.c)

target_link_libraries(triangle Vulkan::Vulkan glfw3)

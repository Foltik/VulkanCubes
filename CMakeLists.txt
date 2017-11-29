cmake_minimum_required(VERSION 3.8)
project(openminer)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og -ggdb")

set(SOURCE_FILES src/Main.cpp src/Threads/concurrentqueue.h src/Threads/Scheduler.h src/Engine.cpp src/Engine.h src/Frames/Frame.h src/Context.cpp src/Context.h src/Window.cpp src/Window.h src/Shader/Shader.cpp src/Shader/Shader.h src/Vulkan/Instance.h src/Vulkan/Structure.h src/Vulkan/VkTraits.h src/Vulkan/Util.h)
add_executable(openminer ${SOURCE_FILES})

target_link_libraries(openminer pthread vulkan glfw)

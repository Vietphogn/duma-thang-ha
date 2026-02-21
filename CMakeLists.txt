cmake_minimum_required(VERSION 3.21)

project(vk_engine
    VERSION 0.1.0
    LANGUAGES CXX
)

# =====================
# Global settings
# =====================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Option build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# =====================
# Dependencies
# =====================
find_package(Vulkan REQUIRED)
add_subdirectory(external/glfw)

# =====================
# Engine sources
# =====================
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

# =====================
# Engine library
# =====================
add_library(engine STATIC ${ENGINE_SOURCES})

target_include_directories(engine
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(engine
    PUBLIC
        Vulkan::Vulkan
        glfw
)

# =====================
# Compile options (performance-focused)
# =====================
target_compile_options(engine PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Release optimization
target_compile_options(engine PRIVATE
    $<$<CONFIG:Release>:-O3>
)

# =====================
# Main executable
# =====================
add_executable(app main.cpp)

target_link_libraries(app PRIVATE engine)

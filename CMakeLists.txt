cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FastBSP "Nvidia dragon taking time to build with a BSP algorithm (871414 triangles),
                use the Suzanne model version (<1000 triangles) instead" OFF)
option(BuildBSP "Build the executable which saves a BSP tree into a binary file" OFF)

if(BuildBSP)
    set(VCPKG_MANIFEST_FEATURES build-bsp)
endif()

project(opengl-transparency VERSION 1.0.0 LANGUAGES CXX)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(FreeGLUT CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(CMakeRC CONFIG REQUIRED)
if(BuildBSP)
    find_package(TBB CONFIG REQUIRED)
endif()

set(BINARY_DIR ${CMAKE_SOURCE_DIR}/${CMAKE_BUILD_TYPE}/bin)
add_subdirectory(media)
add_subdirectory(src)

if(BuildBSP)
    add_subdirectory(src/SaveBspTree)
endif()

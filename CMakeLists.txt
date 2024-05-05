cmake_minimum_required(VERSION 3.16)

project(opengl-transparency VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GLEW REQUIRED)
find_package(FreeGLUT CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

include_directories(common/include)

if(WIN32)
    add_definitions(-DNOMINMAX)
endif()
add_subdirectory(src)

cmake_minimum_required(VERSION 3.16)

project(opengl-transparency VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(thirdparty)

add_definitions(-DGLEW_STATIC)
include_directories(thirdparty/glew/include)
include_directories(thirdparty/freeglut/include)

include_directories(common/nvModel)
include_directories(common/include)
add_subdirectory(common/nvModel)

if(WIN32)
    add_definitions(-DNOMINMAX)
endif()
add_subdirectory(src)

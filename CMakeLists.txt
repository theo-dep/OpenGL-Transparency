cmake_minimum_required(VERSION 3.16)

project(opengl-transparency VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

execute_process(
    COMMAND ${CMAKE_MAKE_PROGRAM} extensions
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/thirdparty/glew
    OUTPUT_QUIET
)
add_definitions(-DGLEW_STATIC)
include_directories(thirdparty/glew/include)
option(BUILD_UTILS "" OFF)
add_subdirectory(thirdparty/glew/build/cmake)

include_directories(thirdparty/freeglut/include)
option(FREEGLUT_BUILD_STATIC_LIBS "" ON)
option(FREEGLUT_BUILD_SHARED_LIBS "" OFF)
option(FREEGLUT_BUILD_DEMOS "" OFF)
add_subdirectory(thirdparty/freeglut)

include_directories(common/nvModel)
include_directories(common/include)
add_subdirectory(common/nvModel)

add_subdirectory(src)

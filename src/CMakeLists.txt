set(TARGET opengl-transparency)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})

if(FastBSP)
    add_definitions(-DBUILD_BSP)
endif()

file(GLOB SHADERS shaders/*.glsl)
cmrc_add_resource_library(shaders-resources
    ALIAS shaders::rc
    NAMESPACE shaders
    ${SHADERS}
)

set(SOURCES
    thirdparty/bsptree.hpp
    VertexBspTree.hpp VertexBspTree.cpp
    GLSLProgramObject.h GLSLProgramObject.cpp
    Mesh.h Mesh.cpp
    OSD.h OSD.cpp
    opengl-transparency.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET} PRIVATE
    $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static>
    GLEW::GLEW
    glm::glm
    assimp::assimp
    Freetype::Freetype
    shaders::rc
)

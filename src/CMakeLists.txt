set(TARGET opengl-transparency)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})

file(GLOB SHADERS shaders/*.glsl)
cmrc_add_resource_library(shaders-resources
    ALIAS shaders::rc
    NAMESPACE shaders
    ${SHADERS}
)

add_executable(${TARGET}
    GLSLProgramObject.h GLSLProgramObject.cpp
    Mesh.h Mesh.cpp
    OSD.h OSD.cpp
    opengl-transparency.cpp
)

target_link_libraries(${TARGET} PRIVATE
    $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static>
    GLEW::GLEW
    glm::glm
    assimp::assimp
    Freetype::Freetype
    shaders::rc
)

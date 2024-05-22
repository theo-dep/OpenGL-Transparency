set(TARGET opengl-transparency)
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
)

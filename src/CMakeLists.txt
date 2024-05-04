set(TARGET opengl-transparency)
add_executable(${TARGET}
    GLSLProgramObject.h GLSLProgramObject.cpp
    OSD.h OSD.cpp
    Timer.h
    opengl-transparency.cpp
)

target_link_libraries(${TARGET} PRIVATE nvModel freeglut_static glew_s)
if (NOT WIN32)
    target_link_libraries(${TARGET} PRIVATE GLU)
endif()

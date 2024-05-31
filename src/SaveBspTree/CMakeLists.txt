set(TARGET build-save-bsp-tree)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})

add_definitions(-DNO_OPENGL)

include_directories(..)

set(SOURCES
    ../thirdparty/bsptree.hpp
    ../VertexBspTree.hpp ../VertexBspTree.cpp
    ../Mesh.h ../Mesh.cpp
    main.cpp
)

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET} PRIVATE assimp::assimp)

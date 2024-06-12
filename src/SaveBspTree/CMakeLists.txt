set(TARGET build-save-bsp-tree)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})

add_definitions(-DNO_OPENGL -DPARALLEL -DPRINT_LOG)

include_directories(..)

set(SOURCES
    ../thirdparty/bsptree.hpp
    ../VertexBspTree.hpp ../VertexBspTree.cpp
    VertexPartBspTree.h VertexPartBspTree.cpp
    ../Mesh.h ../Mesh.cpp
    main.cpp
)

if(WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")
endif()

add_executable(${TARGET} ${SOURCES})

target_link_libraries(${TARGET} PRIVATE assimp::assimp TBB::tbb TBB::tbbmalloc)

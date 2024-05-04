message("-- Searching for glew sources")

if(MSVC)

    include(FetchContent)
    option(CMAKE_TLS_VERIFY "Verify SSL certificates" ON)

    FetchContent_Declare(glew
        URL https://github.com/nigels-com/glew/releases/download/glew-2.1.0/glew-2.1.0.zip
        DOWNLOAD_NO_EXTRACT true
    )
    FetchContent_MakeAvailable(glew)

    set(glewSourceDir ${CMAKE_BINARY_DIR}/_deps/glew-src)
    file(ARCHIVE_EXTRACT
        INPUT ${glewSourceDir}/glew-2.1.0.zip
        DESTINATION ${glewSourceDir}
    )

    function(copy_files_recursively SOURCE DEST)
        file(GLOB FILES "${SOURCE}/*")
        foreach(FILE ${FILES})
            if(IS_DIRECTORY ${FILE})
                get_filename_component(DIRNAME ${FILE} NAME)
                file(MAKE_DIRECTORY "${DEST}/${DIRNAME}")
                copy_files_recursively(${FILE} "${DEST}/${DIRNAME}")
            else()
                get_filename_component(FILENAME ${FILE} NAME)
                if(NOT EXISTS "${DEST}/${FILENAME}")
                    configure_file(${FILE} "${DEST}/${FILENAME}" COPYONLY)
                endif()
            endif()
        endforeach()
    endfunction()

    copy_files_recursively(${glewSourceDir}/glew-2.1.0 ${CMAKE_SOURCE_DIR}/thirdparty/glew)

else()

    execute_process(
        COMMAND ${CMAKE_MAKE_PROGRAM} extensions
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/thirdparty/glew
        COMMAND_ECHO STDOUT
        ENCODING UTF8
        OUTPUT_QUIET
    )

endif()

option(BUILD_UTILS "" OFF)
add_subdirectory(glew/build/cmake)

option(FREEGLUT_BUILD_STATIC_LIBS "" ON)
option(FREEGLUT_BUILD_SHARED_LIBS "" OFF)
option(FREEGLUT_BUILD_DEMOS "" OFF)
add_subdirectory(freeglut)

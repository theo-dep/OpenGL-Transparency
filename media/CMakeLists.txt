set(MODEL dragon)
if(FastBSP)
    # BSP tree needs more than 1 day to compile with the NVidia dragon
    set(MODEL suzanne)
endif()

message("-- Build the sample with the ${MODEL}.obj model")

configure_file(models/${MODEL}.obj ${BINARY_DIR}/models/mesh.obj COPYONLY)
configure_file(fonts/Antonio-Regular.ttf ${BINARY_DIR}/fonts/Antonio-Regular.ttf COPYONLY)

To successfully run this demo example,

we have to include ext/fs/nffs/include directory

into the list of include directories by editing $(zephyr_base)/CMakeLists.txt


##################### For Example ##########################

zephyr_include_directories(

  kernel/include

  arch/${ARCH}/include

  arch/${ARCH}/soc/${SOC_PATH}

  arch/${ARCH}/soc/${SOC_PATH}/include

  arch/${ARCH}/soc/${SOC_FAMILY}/include

  ${BOARD_DIR}

  include

  include/drivers

  ${PROJECT_BINARY_DIR}/include/generated

  ${USERINCLUDE}

  ${STDINCLUDE}

  ext/fs/nffs/include    ######## <<-----------------------

)


##########################################################.


Description:

Press Button 2 on nRF52840_PDK board, so that file with "0.txt" name get created.

Press Button 1 to read data on newly created file.

# To avoid a lot of empty CMakeLists.txt files we assume it is not an
# error if it is missing

if(EXISTS ${BOARD_DIR}/CMakeLists.txt)
  if(BOARD_ROOT)
    add_subdirectory(${BOARD_DIR} boards/${ARCH}/${BOARD})
  else()
    add_subdirectory(${BOARD_DIR})
  endif()
endif()

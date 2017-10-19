# To avoid a lot of empty CMakeLists.txt files we assume it is not an
# error if it is missing

if(EXISTS ${BOARD_DIR}/CMakeLists.txt)
  add_subdirectory(${BOARD_DIR})
endif()

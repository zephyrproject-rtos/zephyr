# To avoid a lot of empty CMakeLists.txt files we assume it is not an
# error if it is missing
set(BOARD_DIR ${ARCH}/${BOARD_NAME})

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${BOARD_DIR}/CMakeLists.txt)
  add_subdirectory(${BOARD_DIR})
endif()

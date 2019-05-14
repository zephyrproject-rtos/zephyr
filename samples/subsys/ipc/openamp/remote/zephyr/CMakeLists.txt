# SPDX-License-Identifier: Apache-2.0

if(FIRST_BOILERPLATE_EXECUTION)
  zephyr_add_image(remote output_variable)

  if(output_variable)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/.. ${CMAKE_BINARY_DIR}/remote)
  endif()
endif()

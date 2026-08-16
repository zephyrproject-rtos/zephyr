# Copyright 2026 MASSDRIVER EI (massdriver.space)
#
# SPDX-License-Identifier: Apache-2.0

function(blob_objcopy name library)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}
    COMMAND ${CMAKE_OBJCOPY} ${ARGN} ${library} ${name}
  )
  add_custom_target(${name}_target DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${name})
  add_dependencies(zephyr_interface ${name}_target)
  zephyr_link_libraries(${CMAKE_CURRENT_BINARY_DIR}/${name})
endfunction()

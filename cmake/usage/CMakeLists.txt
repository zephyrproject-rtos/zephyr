# SPDX-License-Identifier: Apache-2.0

add_custom_target(
  usage
  ${CMAKE_COMMAND}
  -DZEPHYR_BASE=${ZEPHYR_BASE}
  -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/usage.cmake
  )

# NB: The reason it is 'usage' and not help is that CMake already
# defines a target 'help'

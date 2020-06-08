# SPDX-License-Identifier: Apache-2.0

string(REPLACE ";" " " BOARD_ROOT_SPACE_SEPARATED "${BOARD_ROOT}")
string(REPLACE ";" " " SHIELD_LIST_SPACE_SEPARATED "${SHIELD_LIST}")

add_custom_target(
  usage
  ${CMAKE_COMMAND}
  -DZEPHYR_BASE=${ZEPHYR_BASE}
  -DBOARD_ROOT_SPACE_SEPARATED=${BOARD_ROOT_SPACE_SEPARATED}
  -DSHIELD_LIST_SPACE_SEPARATED=${SHIELD_LIST_SPACE_SEPARATED}
  -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/usage.cmake
  )

# NB: The reason it is 'usage' and not help is that CMake already
# defines a target 'help'

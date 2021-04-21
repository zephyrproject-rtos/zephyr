# SPDX-License-Identifier: Apache-2.0

include (${ZEPHYR_BASE}/cmake/shields.cmake)
include (${ZEPHYR_BASE}/cmake/boards.cmake)

# shields.cmake and boards.cmake can be run with cmake -P for printing
# help output on user error when settings BOARD or SHIELD, and
# add_custom_target() is not available in script mode, so we place
# them in here.
add_custom_target(shields ${sorted_shield_cmds} USES_TERMINAL)
add_custom_target(boards ${list_boards_commands} USES_TERMINAL)

add_custom_target(
  usage
  ${CMAKE_COMMAND}
  -DZEPHYR_BASE=${ZEPHYR_BASE}
  -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
  -P ${CMAKE_CURRENT_SOURCE_DIR}/usage.cmake
  )

# NB: The reason it is 'usage' and not help is that CMake already
# defines a target 'help'

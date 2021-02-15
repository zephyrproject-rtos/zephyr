# SPDX-License-Identifier: Apache-2.0

set(arch_root_args)
foreach(root ${ARCH_ROOT})
  list(APPEND arch_root_args "--arch-root=${root}")
endforeach()

set(board_root_args)
foreach(root ${BOARD_ROOT})
  list(APPEND board_root_args "--board-root=${root}")
endforeach()

set(list_boards_commands
  COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/list_boards.py
          ${arch_root_args} ${board_root_args}
)

if(CMAKE_SCRIPT_MODE_FILE AND NOT CMAKE_PARENT_LIST_FILE)
# If this file is invoked as a script directly with -P:
# cmake [options] -P board.cmake
# Note that CMAKE_PARENT_LIST_FILE not being set ensures that this present
# file is being invoked directly with -P, and not via an include directive from
# some other script

# The options available are:
# ARCH_ROOT: Semi-colon separated arch roots
# BOARD_ROOT: Semi-colon separated board roots
# FILE_OUT: Set to a file path to save the boards to a file. If not defined the
#           the contents will be printed to stdout
cmake_minimum_required(VERSION 3.13.1)

set(NO_BOILERPLATE TRUE)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

if (FILE_OUT)
  list(APPEND list_boards_commands OUTPUT_FILE "${FILE_OUT}")
endif()

execute_process(${list_boards_commands})
endif()

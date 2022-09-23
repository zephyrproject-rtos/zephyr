# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Validate board and setup boards target.
#
# This CMake module will validate the BOARD argument as well as splitting the
# BOARD argument into <BOARD> and <BOARD_REVISION>.
#
# If a board implementation is not found for the specified board an error will
# be raised and list of valid boards will be printed.
#
# If user provided board is a board alias, the board will be adjusted to real
# board name.
#
# If board name is deprecated, then board will be adjusted to new board name and
# a deprecation warning will be printed to the user.
#
# Outcome:
# The following variables will be defined when this CMake module completes:
#
# - BOARD:          Board, without revision field.
# - BOARD_REVISION: Board revision
# - BOARD_DIR:      Board directory with the implementation for selected board
# - ARCH_DIR:       Arch dir for extracted from selected board
# - BOARD_ROOT:     BOARD_ROOT with ZEPHYR_BASE appended
#
# The following targets will be defined when this CMake module completes:
# - board  : when invoked a list of valid boards will be printed
#
# Required variables:
# - BOARD: Board name, including any optional revision field, for example: `foo` or `foo@1.0.0`
#
# Optional variables:
# - BOARD_ROOT: CMake list of board roots containing board implementations
# - ARCH_ROOT:  CMake list of arch roots containing arch implementations
#
# Optional environment variables:
# - ZEPHYR_BOARD_ALIASES: Environment setting pointing to a CMake file
#                         containing board aliases.
#
# Variables set by this module and not mentioned above are considered internal
# use only and may be removed, renamed, or re-purposed without prior notice.

include_guard(GLOBAL)

include(python)
include(extensions)

# Check that BOARD has been provided, and that it has not changed.
# If user tries to change the BOARD, the BOARD value is reset to the BOARD_CACHED value.
zephyr_check_cache(BOARD REQUIRED)

# 'BOARD_ROOT' is a prioritized list of directories where boards may
# be found. It always includes ${ZEPHYR_BASE} at the lowest priority (except for unittesting).
if(NOT unittest IN_LIST Zephyr_FIND_COMPONENTS)
  list(APPEND BOARD_ROOT ${ZEPHYR_BASE})
endif()

string(FIND "${BOARD}" "@" REVISION_SEPARATOR_INDEX)
if(NOT (REVISION_SEPARATOR_INDEX EQUAL -1))
  math(EXPR BOARD_REVISION_INDEX "${REVISION_SEPARATOR_INDEX} + 1")
  string(SUBSTRING ${BOARD} ${BOARD_REVISION_INDEX} -1 BOARD_REVISION)
  string(SUBSTRING ${BOARD} 0 ${REVISION_SEPARATOR_INDEX} BOARD)
endif()

zephyr_get(ZEPHYR_BOARD_ALIASES)
if(DEFINED ZEPHYR_BOARD_ALIASES)
  include(${ZEPHYR_BOARD_ALIASES})
  if(${BOARD}_BOARD_ALIAS)
    set(BOARD_ALIAS ${BOARD} CACHE STRING "Board alias, provided by user")
    set(BOARD ${${BOARD}_BOARD_ALIAS})
    message(STATUS "Aliased BOARD=${BOARD_ALIAS} changed to ${BOARD}")
  endif()
endif()
include(${ZEPHYR_BASE}/boards/deprecated.cmake)
if(${BOARD}_DEPRECATED)
  set(BOARD_DEPRECATED ${BOARD} CACHE STRING "Deprecated board name, provided by user")
  set(BOARD ${${BOARD}_DEPRECATED})
  message(WARNING "Deprecated BOARD=${BOARD_DEPRECATED} name specified, board automatically changed to: ${BOARD}.")
endif()

zephyr_boilerplate_watch(BOARD)

foreach(root ${BOARD_ROOT})
  # Check that the board root looks reasonable.
  if(NOT IS_DIRECTORY "${root}/boards")
    message(WARNING "BOARD_ROOT element without a 'boards' subdirectory:
${root}
Hints:
  - if your board directory is '/foo/bar/boards/<ARCH>/my_board' then add '/foo/bar' to BOARD_ROOT, not the entire board directory
  - if in doubt, use absolute paths")
  endif()

  # NB: find_path will return immediately if the output variable is
  # already set
  if (BOARD_ALIAS)
    find_path(BOARD_HIDDEN_DIR
      NAMES ${BOARD_ALIAS}_defconfig
      PATHS ${root}/boards/*/*
      NO_DEFAULT_PATH
      )
    if(BOARD_HIDDEN_DIR)
      message("Board alias ${BOARD_ALIAS} is hiding the real board of same name")
    endif()
  endif()
  if(BOARD_DIR AND NOT EXISTS ${BOARD_DIR}/${BOARD}_defconfig)
    message(WARNING "BOARD_DIR: ${BOARD_DIR} has been moved or deleted. "
                    "Trying to find new location."
    )
    set(BOARD_DIR BOARD_DIR-NOTFOUND CACHE PATH "Path to a file." FORCE)
  endif()
  find_path(BOARD_DIR
    NAMES ${BOARD}_defconfig
    PATHS ${root}/boards/*/*
    NO_DEFAULT_PATH
    )
  if(BOARD_DIR AND NOT (${root} STREQUAL ${ZEPHYR_BASE}))
    set(USING_OUT_OF_TREE_BOARD 1)
  endif()
endforeach()

if(EXISTS ${BOARD_DIR}/revision.cmake)
  # Board provides revision handling.
  include(${BOARD_DIR}/revision.cmake)
elseif(BOARD_REVISION)
  message(WARNING "Board revision ${BOARD_REVISION} specified for ${BOARD}, \
                   but board has no revision so revision will be ignored.")
endif()

set(board_message "Board: ${BOARD}")

if(DEFINED BOARD_REVISION)
  set(board_message "${board_message}, Revision: ${BOARD_REVISION}")
  if(DEFINED ACTIVE_BOARD_REVISION)
    set(board_message "${board_message} (Active: ${ACTIVE_BOARD_REVISION})")
    set(BOARD_REVISION ${ACTIVE_BOARD_REVISION})
  endif()

  string(REPLACE "." "_" BOARD_REVISION_STRING ${BOARD_REVISION})
endif()

message(STATUS "${board_message}")

# Prepare boards usage command printing.
# This command prints all boards in the system in the following cases:
# - User specifies an invalid BOARD
# - User invokes '<build-command> boards' target
list(TRANSFORM ARCH_ROOT PREPEND "--arch-root=" OUTPUT_VARIABLE arch_root_args)
list(TRANSFORM BOARD_ROOT PREPEND "--board-root=" OUTPUT_VARIABLE board_root_args)

set(list_boards_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/list_boards.py
            ${arch_root_args} ${board_root_args} --arch-root=${ZEPHYR_BASE}
)

if(NOT BOARD_DIR)
  message("No board named '${BOARD}' found.\n\n"
          "Please choose one of the following boards:\n"
  )
  execute_process(${list_boards_commands})
  unset(CACHED_BOARD CACHE)
  message(FATAL_ERROR "Invalid BOARD; see above.")
endif()

add_custom_target(boards ${list_boards_commands} USES_TERMINAL)

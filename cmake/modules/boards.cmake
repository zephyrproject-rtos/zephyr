# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2021, Nordic Semiconductor ASA

# Validate board and setup boards target.
#
# This CMake module will validate the BOARD argument as well as splitting the
# BOARD argument into <BOARD> and <BOARD_REVISION>. When BOARD_EXTENSIONS option
# is enabled (default) this module will also take care of finding board
# extension directories.
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
# - BOARD:                       Board, without revision field.
# - BOARD_REVISION:              Board revision
# - BOARD_QUALIFIERS:            Board qualifiers
# - NORMALIZED_BOARD_QUALIFIERS: Board qualifiers in lower-case format where slashes have been
#                                replaced with underscores
# - NORMALIZED_BOARD_TARGET:     Board target in lower-case format where slashes have been
#                                replaced with underscores
# - BOARD_DIR:                   Board directory with the implementation for selected board
# - ARCH_DIR:                    Arch dir for extracted from selected board
# - BOARD_ROOT:                  BOARD_ROOT with ZEPHYR_BASE appended
# - BOARD_EXTENSION_DIRS:        List of board extension directories (If
#                                BOARD_EXTENSIONS is not explicitly disabled)
#
# The following targets will be defined when this CMake module completes:
# - board: when invoked, a list of valid boards will be printed
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
# Variables set by this module and not mentioned above are for internal
# use only, and may be removed, renamed, or re-purposed without prior notice.

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

# Helper function for parsing a board's name, revision, and qualifiers,
# from one input variable to three separate output variables.
function(parse_board_components board_in name_out revision_out qualifiers_out)
  if(NOT "${${board_in}}" MATCHES "^([^@/]+)(@[^@/]+)?(/[^@]+)?$")
    message(FATAL_ERROR
      "Invalid revision / qualifiers format for ${board_in} (${${board_in}}). "
      "Valid format is: <board>@<revision>/<qualifiers>"
    )
  endif()
  string(REPLACE "@" "" board_revision "${CMAKE_MATCH_2}")

  set(${name_out}       ${CMAKE_MATCH_1}  PARENT_SCOPE)
  set(${revision_out}   ${board_revision} PARENT_SCOPE)
  set(${qualifiers_out} ${CMAKE_MATCH_3}  PARENT_SCOPE)
endfunction()

parse_board_components(
  BOARD
  BOARD BOARD_REVISION BOARD_QUALIFIERS
)

zephyr_get(ZEPHYR_BOARD_ALIASES)
if(DEFINED ZEPHYR_BOARD_ALIASES)
  include(${ZEPHYR_BOARD_ALIASES})
  if(${BOARD}_BOARD_ALIAS)
    set(BOARD_ALIAS ${BOARD} CACHE STRING "Board alias, provided by user")
    parse_board_components(
      ${BOARD}_BOARD_ALIAS
      BOARD BOARD_ALIAS_REVISION BOARD_ALIAS_QUALIFIERS
    )
    message(STATUS "Aliased BOARD=${BOARD_ALIAS} changed to ${BOARD}")
    if(NOT DEFINED BOARD_REVISION)
      set(BOARD_REVISION ${BOARD_ALIAS_REVISION})
    endif()
    set(BOARD_QUALIFIERS ${BOARD_ALIAS_QUALIFIERS}${BOARD_QUALIFIERS})
  endif()
endif()

include(${ZEPHYR_BASE}/boards/deprecated.cmake)
if(${BOARD}${BOARD_QUALIFIERS}_DEPRECATED)
  set(BOARD_DEPRECATED ${BOARD}${BOARD_QUALIFIERS} CACHE STRING "Deprecated BOARD, provided by user")
  message(WARNING
    "Deprecated BOARD=${BOARD_DEPRECATED} specified, "
    "board automatically changed to: ${${BOARD}${BOARD_QUALIFIERS}_DEPRECATED}."
  )
  parse_board_components(
    ${BOARD}${BOARD_QUALIFIERS}_DEPRECATED
    BOARD BOARD_DEPRECATED_REVISION BOARD_QUALIFIERS
  )
  if(DEFINED BOARD_DEPRECATED_REVISION)
    if(DEFINED BOARD_REVISION)
      message(FATAL_ERROR
        "Invalid board revision: ${BOARD_REVISION}\n"
        "Deprecated board '${BOARD_DEPRECATED}' is now implemented as a revision of another board "
        "(${BOARD}@${BOARD_DEPRECATED_REVISION}), so the specified revision does not apply. "
        "Please consult the documentation for '${BOARD}' to see how to build for the new board."
      )
    endif()
    set(BOARD_REVISION ${BOARD_DEPRECATED_REVISION})
  endif()
endif()

zephyr_boilerplate_watch(BOARD)

foreach(root ${BOARD_ROOT})
  # Check that the board root looks reasonable.
  if(NOT IS_DIRECTORY "${root}/boards")
    message(WARNING "BOARD_ROOT element without a 'boards' subdirectory:
${root}
Hints:
  - if your board directory is '/foo/bar/boards/my_board' then add '/foo/bar' to BOARD_ROOT, not the entire board directory
  - if in doubt, use absolute paths")
  endif()
endforeach()

if((HWMv1 AND NOT EXISTS ${BOARD_DIR}/${BOARD}_defconfig)
   OR (HWMv2 AND NOT EXISTS ${BOARD_DIR}/board.yml))
  message(WARNING "BOARD_DIR: ${BOARD_DIR} has been moved or deleted. "
                  "Trying to find new location."
  )
  set(BOARD_DIR BOARD_DIR-NOTFOUND CACHE PATH "Path to a file." FORCE)
endif()

# Prepare list boards command.
# This command is used for locating the board dir as well as printing all boards
# in the system in the following cases:
# - User specifies an invalid BOARD
# - User invokes '<build-command> boards' target
list(TRANSFORM ARCH_ROOT PREPEND "--arch-root=" OUTPUT_VARIABLE arch_root_args)
list(TRANSFORM BOARD_ROOT PREPEND "--board-root=" OUTPUT_VARIABLE board_root_args)
list(TRANSFORM SOC_ROOT PREPEND "--soc-root=" OUTPUT_VARIABLE soc_root_args)

set(list_boards_commands
    COMMAND ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/list_boards.py
            ${arch_root_args} ${board_root_args} --arch-root=${ZEPHYR_BASE}
            ${soc_root_args} --soc-root=${ZEPHYR_BASE}
)

if(NOT BOARD_DIR)
  if(BOARD_ALIAS)
    execute_process(${list_boards_commands} --board=${BOARD_ALIAS} --cmakeformat={DIR}
                    OUTPUT_VARIABLE ret_board
                    ERROR_VARIABLE err_board
                    RESULT_VARIABLE ret_val
    )
    string(STRIP "${ret_board}" ret_board)
    cmake_parse_arguments(BOARD_HIDDEN "" "DIR" "" ${ret_board})
    set(BOARD_HIDDEN_DIR ${BOARD_HIDDEN_DIR} CACHE PATH "Path to a folder." FORCE)

    if(BOARD_HIDDEN_DIR)
      message("Board alias ${BOARD_ALIAS} is hiding the real board of same name")
    endif()
  endif()
endif()

set(format_str "{NAME}\;{DIR}\;{HWM}\;")
set(format_str "${format_str}{REVISION_FORMAT}\;{REVISION_DEFAULT}\;{REVISION_EXACT}\;")
set(format_str "${format_str}{REVISIONS}\;{SOCS}\;{QUALIFIERS}")

if(BOARD_DIR)
  set(board_dir_arg "--board-dir=${BOARD_DIR}")
endif()
execute_process(${list_boards_commands} --board=${BOARD} ${board_dir_arg}
  --cmakeformat=${format_str}
                OUTPUT_VARIABLE ret_board
                ERROR_VARIABLE err_board
                RESULT_VARIABLE ret_val
)
if(ret_val)
  message(FATAL_ERROR "Error finding board: ${BOARD}\nError message: ${err_board}")
endif()

if(NOT "${ret_board}" STREQUAL "")
  string(STRIP "${ret_board}" ret_board)
  string(FIND "${ret_board}" "\n" idx REVERSE)
  if(idx GREATER -1)
    while(TRUE)
      math(EXPR start "${idx} + 1")
      string(SUBSTRING "${ret_board}" ${start} -1 line)
      string(SUBSTRING "${ret_board}" 0 ${idx} ret_board)

      cmake_parse_arguments(LIST_BOARD "" "DIR" "" ${line})
      set(board_dirs "${board_dirs}\n${LIST_BOARD_DIR}")

      if(idx EQUAL -1)
        break()
      endif()
      string(FIND "${ret_board}" "\n" idx REVERSE)
    endwhile()
    message(FATAL_ERROR "Multiple boards named '${BOARD}' found in:${board_dirs}")
  endif()

  set(single_val "NAME;DIR;HWM;REVISION_FORMAT;REVISION_DEFAULT;REVISION_EXACT")
  set(multi_val  "REVISIONS;SOCS;QUALIFIERS")
  cmake_parse_arguments(LIST_BOARD "" "${single_val}" "${multi_val}" ${ret_board})
  set(BOARD_DIR ${LIST_BOARD_DIR} CACHE PATH "Board directory for board (${BOARD})" FORCE)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BOARD_DIR}/board.yml)

  # Create two CMake variables identifying the hw model.
  # CMake variable: HWM=[v1,v2]
  # CMake variable: HWMv1=True, when HWMv1 is in use.
  # CMake variable: HWMv2=True, when HWMv2 is in use.
  set(HWM       ${LIST_BOARD_HWM} CACHE INTERNAL "Zephyr hardware model version")
  set(HWM${HWM} True   CACHE INTERNAL "Zephyr hardware model")
elseif(BOARD_DIR)
  message(FATAL_ERROR "Error finding board: ${BOARD} in ${BOARD_DIR}.\n"
          "This indicates the board has been removed, renamed, or placed at a new location.\n"
	  "Please run a pristine build."
  )
else()
  message("No board named '${BOARD}' found.\n\n"
          "Please choose one of the following boards:\n"
  )
  execute_process(${list_boards_commands})
  unset(CACHED_BOARD CACHE)
  message(FATAL_ERROR "Invalid BOARD; see above.")
endif()

if(HWMv1 AND DEFINED BOARD_QUALIFIERS)
  message(FATAL_ERROR
          "Board '${BOARD}' does not support board qualifiers, ${BOARD}${BOARD_QUALIFIERS}.\n"
          "Please specify board without qualifiers.\n"
  )
endif()

cmake_path(IS_PREFIX ZEPHYR_BASE "${BOARD_DIR}" NORMALIZE in_zephyr_tree)
if(NOT in_zephyr_tree)
  set(USING_OUT_OF_TREE_BOARD 1)
endif()

if(HWMv1)
  if(EXISTS ${BOARD_DIR}/revision.cmake)
    # Board provides revision handling.
    include(${BOARD_DIR}/revision.cmake)
  elseif(BOARD_REVISION)
    message(WARNING "Board revision ${BOARD_REVISION} specified for ${BOARD}, \
                     but board has no revision so revision will be ignored.")
  endif()
elseif(HWMv2)
  if(LIST_BOARD_REVISION_FORMAT)
    if(LIST_BOARD_REVISION_FORMAT STREQUAL "custom")
      include(${BOARD_DIR}/revision.cmake)
    else()
      if(EXISTS ${BOARD_DIR}/revision.cmake)
        message(WARNING
          "revision.cmake ignored, revision.cmake is only used for revision format: 'custom'"
        )
      endif()

      string(TOUPPER "${LIST_BOARD_REVISION_FORMAT}" rev_format)
      if(LIST_BOARD_REVISION_EXACT)
        set(rev_exact EXACT)
      endif()

      board_check_revision(
        FORMAT ${rev_format}
        DEFAULT_REVISION ${LIST_BOARD_REVISION_DEFAULT}
        VALID_REVISIONS ${LIST_BOARD_REVISIONS}
        ${rev_exact}
      )
    endif()
  elseif(DEFINED BOARD_REVISION)
    if(EXISTS ${BOARD_DIR}/revision.cmake)
      message(WARNING
        "revision.cmake is not used, revisions must be defined in '${BOARD_DIR}/board.yml'"
      )
    endif()

    message(FATAL_ERROR "Invalid board revision: ${BOARD_REVISION}\n"
                        "Board '${BOARD}' does not define any revisions."
    )
  endif()

  if(LIST_BOARD_QUALIFIERS)
    # Allow users to omit the SoC when building for a board with a single SoC.
    list(LENGTH LIST_BOARD_SOCS socs_length)
    if(socs_length EQUAL 1)
      set(BOARD_SINGLE_SOC TRUE)
      set(BOARD_${BOARD}_SINGLE_SOC TRUE)
      if(NOT DEFINED BOARD_QUALIFIERS)
        set(BOARD_QUALIFIERS "/${LIST_BOARD_SOCS}")
      elseif("${BOARD_QUALIFIERS}" MATCHES "^//.*")
        string(REGEX REPLACE "^//" "/${LIST_BOARD_SOCS}/" BOARD_QUALIFIERS "${BOARD_QUALIFIERS}")
      endif()
    endif()

    set(board_targets ${LIST_BOARD_QUALIFIERS})
    list(TRANSFORM board_targets PREPEND "${BOARD}/")
    if(NOT ("${BOARD}${BOARD_QUALIFIERS}" IN_LIST board_targets))
      string(REPLACE ";" "\n" board_targets "${board_targets}")
      unset(CACHED_BOARD CACHE)
      message(FATAL_ERROR "Board qualifiers `${BOARD_QUALIFIERS}` for board \
            `${BOARD}` not found. Please specify a valid board target.\n"
            "Valid board targets for ${LIST_BOARD_NAME} are:\n${board_targets}\n")
    endif()
  endif()
else()
  message(FATAL_ERROR "Unknown hw model (${HWM}) for board: ${BOARD}.")
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

if(DEFINED BOARD_QUALIFIERS)
  string(REGEX REPLACE "^/" "qualifiers: " board_message_qualifiers "${BOARD_QUALIFIERS}")
  set(board_message "${board_message}, ${board_message_qualifiers}")

  string(REPLACE "/" "_" NORMALIZED_BOARD_QUALIFIERS "${BOARD_QUALIFIERS}")
endif()

set(NORMALIZED_BOARD_TARGET "${BOARD}${BOARD_QUALIFIERS}")
string(REPLACE "/" "_" NORMALIZED_BOARD_TARGET "${NORMALIZED_BOARD_TARGET}")

message(STATUS "${board_message}")

add_custom_target(boards ${list_boards_commands} USES_TERMINAL)

# Board extensions are enabled by default
set(BOARD_EXTENSIONS ON CACHE BOOL "Support board extensions")
zephyr_get(BOARD_EXTENSIONS)

# Process board extensions
if(BOARD_EXTENSIONS)
  get_filename_component(board_dir_name ${BOARD_DIR} NAME)

  foreach(root ${BOARD_ROOT})
    set(board_extension_dir ${root}/boards/extensions/${board_dir_name})
    if(NOT EXISTS ${board_extension_dir})
      continue()
    endif()

    list(APPEND BOARD_EXTENSION_DIRS ${board_extension_dir})
  endforeach()
endif()

# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2024, The MathWorks, Inc.

include(boards)
include(git)
include(extensions)
include(west)

# find Polyspace, stop if missing
find_program(POLYSPACE_CONFIGURE_EXE NAMES polyspace-configure)
if(NOT POLYSPACE_CONFIGURE_EXE)
  message(FATAL_ERROR "Polyspace not found. For install instructions, see https://mathworks.com/help/bugfinder/install")
else()
  cmake_path(GET POLYSPACE_CONFIGURE_EXE PARENT_PATH POLYSPACE_BIN)
  message(STATUS "Found SCA: Polyspace (${POLYSPACE_BIN})")
endif()
find_program(POLYSPACE_RESULTS_EXE NAMES polyspace-results-export REQUIRED)


# Get Polyspace specific variables
zephyr_get(POLYSPACE_ONLY_APP)
zephyr_get(POLYSPACE_CONFIGURE_OPTIONS)
zephyr_get(POLYSPACE_MODE)
zephyr_get(POLYSPACE_OPTIONS)
zephyr_get(POLYSPACE_OPTIONS_FILE)
zephyr_get(POLYSPACE_PROG_NAME)
zephyr_get(POLYSPACE_PROG_VERSION)
message(TRACE "POLYSPACE_OPTIONS is ${POLYSPACE_OPTIONS}")


# Get path and name of user application
zephyr_get(APPLICATION_SOURCE_DIR)
cmake_path(GET APPLICATION_SOURCE_DIR FILENAME APPLICATION_NAME)
message(TRACE "APPLICATION_SOURCE_DIR is ${APPLICATION_SOURCE_DIR}")
message(TRACE "APPLICATION_NAME is ${APPLICATION_NAME}")


# process options
if(POLYSPACE_ONLY_APP)
  set(POLYSPACE_CONFIGURE_OPTIONS ${POLYSPACE_CONFIGURE_OPTIONS} -include-sources ${APPLICATION_SOURCE_DIR}/**)
  message(WARNING "SCA only analyzes application code")
endif()
if(POLYSPACE_MODE STREQUAL "prove")
  message(NOTICE "POLYSPACE in proof mode")
  find_program(POLYSPACE_EXE NAMES polyspace-code-prover-server polyspace-code-prover)
else()
  message(NOTICE "POLYSPACE in bugfinding mode")
  find_program(POLYSPACE_EXE NAMES polyspace-bug-finder-server polyspace-bug-finder)
endif()
if(NOT POLYSPACE_PROG_NAME)
  set(POLYSPACE_PROG_NAME "zephyr-${BOARD}-${APPLICATION_NAME}")
endif()
message(TRACE "POLYSPACE_PROG_NAME is ${POLYSPACE_PROG_NAME}")
if(POLYSPACE_OPTIONS_FILE)
  message(TRACE "POLYSPACE_OPTIONS_FILE is ${POLYSPACE_OPTIONS_FILE}")
  set(POLYSPACE_OPTIONS_FILE -options-file ${POLYSPACE_OPTIONS_FILE})
endif()
if(POLYSPACE_PROG_VERSION)
  set(POLYSPACE_PROG_VERSION -verif-version '${POLYSPACE_PROG_VERSION}')
else()
  git_describe(${APPLICATION_SOURCE_DIR} app_version)
  if(app_version)
    set(POLYSPACE_PROG_VERSION -verif-version '${app_version}')
  endif()
endif()
message(TRACE "POLYSPACE_PROG_VERSION is ${POLYSPACE_PROG_VERSION}")

# tell Polyspace about Zephyr specials
set(POLYSPACE_OPTIONS_ZEPHYR -options-file ${CMAKE_CURRENT_LIST_DIR}/zephyr.psopts)

# Polyspace requires the compile_commands.json as input
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create results directory
set(POLYSPACE_RESULTS_DIR ${CMAKE_BINARY_DIR}/sca/polyspace)
set(POLYSPACE_RESULTS_FILE ${POLYSPACE_RESULTS_DIR}/results.csv)
file(MAKE_DIRECTORY ${POLYSPACE_RESULTS_DIR})
message(TRACE "POLYSPACE_RESULTS_DIR is ${POLYSPACE_RESULTS_DIR}")
set(POLYSPACE_OPTIONS_FILE_BASE ${POLYSPACE_RESULTS_DIR}/polyspace.psopts)


#####################
# mandatory targets #
#####################

add_custom_target(polyspace_configure ALL
  COMMAND ${POLYSPACE_CONFIGURE_EXE}
    -allow-overwrite
    -silent
    -prog ${POLYSPACE_PROG_NAME}
    -compilation-database ${CMAKE_BINARY_DIR}/compile_commands.json
    -output-options-file ${POLYSPACE_OPTIONS_FILE_BASE}
    ${POLYSPACE_CONFIGURE_OPTIONS}
  VERBATIM
  DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
  BYPRODUCTS ${POLYSPACE_OPTIONS_FILE_BASE}
  USES_TERMINAL
  COMMAND_EXPAND_LISTS
)

add_custom_target(polyspace-analyze ALL
  COMMAND ${POLYSPACE_EXE}
    -results-dir ${POLYSPACE_RESULTS_DIR}
    -options-file ${POLYSPACE_OPTIONS_FILE_BASE}
    ${POLYSPACE_PROG_VERSION}
    ${POLYSPACE_OPTIONS_ZEPHYR}
    ${POLYSPACE_OPTIONS_FILE}
    ${POLYSPACE_OPTIONS}
      || ${CMAKE_COMMAND} -E true # allow to continue processing results
  DEPENDS ${POLYSPACE_OPTIONS_FILE_BASE}
  USES_TERMINAL
  COMMAND_EXPAND_LISTS
)

add_custom_target(polyspace-results ALL
  COMMAND ${POLYSPACE_RESULTS_EXE}
    -results-dir ${POLYSPACE_RESULTS_DIR}
    -output-name ${POLYSPACE_RESULTS_FILE}
    -format csv
      || ${CMAKE_COMMAND} -E true # allow to continue processing results
  VERBATIM
  USES_TERMINAL
  COMMAND_EXPAND_LISTS
)

add_dependencies(polyspace-results polyspace-analyze)


#####################
# summarize results #
#####################

add_custom_command(TARGET polyspace-results POST_BUILD
  COMMAND ${CMAKE_CURRENT_LIST_DIR}/polyspace-print-console.py
  ${POLYSPACE_RESULTS_FILE}
  COMMAND
    ${CMAKE_COMMAND} -E cmake_echo_color --cyan --bold
    "SCA results are here: ${POLYSPACE_RESULTS_DIR}"
  VERBATIM
  USES_TERMINAL
)

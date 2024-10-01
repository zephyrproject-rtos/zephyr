# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023, Basalte bv

include(boards)
include(git)
include(extensions)
include(west)

find_program(CODECHECKER_EXE NAMES CodeChecker codechecker REQUIRED)
message(STATUS "Found SCA: CodeChecker (${CODECHECKER_EXE})")

# Get CodeChecker specific variables
zephyr_get(CODECHECKER_ANALYZE_JOBS)
zephyr_get(CODECHECKER_ANALYZE_OPTS)
zephyr_get(CODECHECKER_CLEANUP)
zephyr_get(CODECHECKER_CONFIG_FILE)
zephyr_get(CODECHECKER_EXPORT)
zephyr_get(CODECHECKER_NAME)
zephyr_get(CODECHECKER_PARSE_EXIT_STATUS)
zephyr_get(CODECHECKER_PARSE_OPTS)
zephyr_get(CODECHECKER_PARSE_SKIP)
zephyr_get(CODECHECKER_STORE)
zephyr_get(CODECHECKER_STORE_OPTS)
zephyr_get(CODECHECKER_STORE_TAG)
zephyr_get(CODECHECKER_TRIM_PATH_PREFIX MERGE VAR CODECHECKER_TRIM_PATH_PREFIX WEST_TOPDIR)

# Get twister runner specific variables
zephyr_get(TC_RUNID)
zephyr_get(TC_NAME)

if(NOT CODECHECKER_NAME)
  if(TC_NAME)
    set(CODECHECKER_NAME "${BOARD}${BOARD_QUALIFIERS}:${TC_NAME}")
  else()
    set(CODECHECKER_NAME zephyr)
  endif()
endif()

if(CODECHECKER_ANALYZE_JOBS)
  set(CODECHECKER_ANALYZE_JOBS "--jobs;${CODECHECKER_ANALYZE_JOBS}")
elseif(TC_RUNID)
  set(CODECHECKER_ANALYZE_JOBS "--jobs;1")
endif()

if(CODECHECKER_CONFIG_FILE)
  set(CODECHECKER_CONFIG_FILE "--config;${CODECHECKER_CONFIG_FILE}")
endif()

if(CODECHECKER_STORE_TAG)
  set(CODECHECKER_STORE_TAG "--tag;${CODECHECKER_STORE_TAG}")
else()
  git_describe(${APPLICATION_SOURCE_DIR} app_version)
  if(app_version)
    set(CODECHECKER_STORE_TAG "--tag;${app_version}")
  endif()
endif()

if(CODECHECKER_TRIM_PATH_PREFIX)
  set(CODECHECKER_TRIM_PATH_PREFIX "--trim-path-prefix;${CODECHECKER_TRIM_PATH_PREFIX}")
endif()

# CodeChecker uses the compile_commands.json as input
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create an output directory for our tool
set(output_dir ${CMAKE_BINARY_DIR}/sca/codechecker)
file(MAKE_DIRECTORY ${output_dir})

# Use a dummy file to let CodeChecker know we can start analyzing
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
  ${CMAKE_COMMAND} -E touch ${output_dir}/codechecker.ready)
set_property(GLOBAL APPEND PROPERTY extra_post_build_byproducts
  ${output_dir}/codechecker.ready)

add_custom_target(codechecker ALL
  COMMAND ${CODECHECKER_EXE} analyze
    --keep-gcc-include-fixed
    --keep-gcc-intrin
    --output ${output_dir}/codechecker.plist
    --name ${CODECHECKER_NAME} # Set a default metadata name
    ${CODECHECKER_CONFIG_FILE}
    ${CODECHECKER_ANALYZE_JOBS}
    ${CODECHECKER_ANALYZE_OPTS}
    ${CMAKE_BINARY_DIR}/compile_commands.json
    || ${CMAKE_COMMAND} -E true # allow to continue processing results
  DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json ${output_dir}/codechecker.ready
  VERBATIM
  USES_TERMINAL
  COMMAND_EXPAND_LISTS
)

# Cleanup dummy file
add_custom_command(
  TARGET codechecker POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E rm ${output_dir}/codechecker.ready
)

if(CODECHECKER_CLEANUP)
  add_custom_target(codechecker-cleanup ALL
    COMMAND ${CMAKE_COMMAND} -E rm -r ${output_dir}/codechecker.plist
  )
else()
  add_custom_target(codechecker-cleanup)
endif()

add_dependencies(codechecker-cleanup codechecker)

# If 'codechecker parse' returns an exit status of '2', it means more than 0
# issues were detected. Suppress the exit status by default, but permit opting
# in to the failure.
if(NOT CODECHECKER_PARSE_EXIT_STATUS)
  set(CODECHECKER_PARSE_OPTS ${CODECHECKER_PARSE_OPTS} || ${CMAKE_COMMAND} -E true)
endif()

if(DEFINED CODECHECKER_EXPORT)
  string(REPLACE "," ";" export_list ${CODECHECKER_EXPORT})

  foreach(export_item IN LISTS export_list)
    message(STATUS "CodeChecker export: ${CMAKE_BINARY_DIR}/codechecker.${export_item}")

    add_custom_target(codechecker-report-${export_item} ALL
      COMMAND ${CODECHECKER_EXE} parse
        ${output_dir}/codechecker.plist
        --export ${export_item}
        --output ${output_dir}/codechecker.${export_item}
        ${CODECHECKER_CONFIG_FILE}
        ${CODECHECKER_TRIM_PATH_PREFIX}
        ${CODECHECKER_PARSE_OPTS}
      BYPRODUCTS ${output_dir}/codechecker.${export_item}
      VERBATIM
      USES_TERMINAL
      COMMAND_EXPAND_LISTS
    )
    add_dependencies(codechecker-report-${export_item} codechecker)
    add_dependencies(codechecker-cleanup codechecker-report-${export_item})
  endforeach()
elseif(NOT CODECHECKER_PARSE_SKIP)
  # Output parse results
    add_custom_target(codechecker-parse ALL
    COMMAND ${CODECHECKER_EXE} parse
      ${output_dir}/codechecker.plist
      ${CODECHECKER_CONFIG_FILE}
      ${CODECHECKER_TRIM_PATH_PREFIX}
      ${CODECHECKER_PARSE_OPTS}
    VERBATIM
    USES_TERMINAL
    COMMAND_EXPAND_LISTS
  )
  add_dependencies(codechecker-parse codechecker)
  add_dependencies(codechecker-cleanup codechecker-parse)
endif()

if(DEFINED CODECHECKER_STORE OR DEFINED CODECHECKER_STORE_OPTS)
  add_custom_target(codechecker-store ALL
    COMMAND ${CODECHECKER_EXE} store
      ${CODECHECKER_CONFIG_FILE}
      ${CODECHECKER_STORE_TAG}
      ${CODECHECKER_TRIM_PATH_PREFIX}
      ${CODECHECKER_STORE_OPTS}
      ${output_dir}/codechecker.plist
    VERBATIM
    USES_TERMINAL
    COMMAND_EXPAND_LISTS
  )
  add_dependencies(codechecker-store codechecker)
  add_dependencies(codechecker-cleanup codechecker-store)
endif()

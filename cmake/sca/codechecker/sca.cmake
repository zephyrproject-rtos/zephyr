# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023, Basalte bv

find_program(CODECHECKER_EXE CodeChecker REQUIRED)
message(STATUS "Found CodeChecker: ${CODECHECKER_EXE}")

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
    --name zephyr # Set a default metadata name
    ${CODECHECKER_ANALYZE_OPTS}
    ${CMAKE_BINARY_DIR}/compile_commands.json
  DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json ${output_dir}/codechecker.ready
  BYPRODUCTS ${output_dir}/codechecker.plist
  VERBATIM
  USES_TERMINAL
  COMMAND_EXPAND_LISTS
)

# Cleanup dummy file
add_custom_command(
  TARGET codechecker POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E rm ${output_dir}/codechecker.ready
)

if(CODECHECKER_EXPORT)
  string(REPLACE "," ";" export_list ${CODECHECKER_EXPORT})

  foreach(export_item IN LISTS export_list)
    message(STATUS "CodeChecker export: ${CMAKE_BINARY_DIR}/codechecker.${export_item}")

    add_custom_command(
      TARGET codechecker POST_BUILD
      COMMAND ${CODECHECKER_EXE} parse
        ${output_dir}/codechecker.plist
        --export ${export_item}
        --output ${output_dir}/codechecker.${export_item}
        ${CODECHECKER_PARSE_OPTS}
        || ${CMAKE_COMMAND} -E true # parse has exit code 2 if a report is emitted by an analyzer
      BYPRODUCTS ${output_dir}/codechecker.${export_item}
      VERBATIM
      USES_TERMINAL
      COMMAND_EXPAND_LISTS
    )
  endforeach()
else()
  # Output parse results
  add_custom_command(
    TARGET codechecker POST_BUILD
    COMMAND ${CODECHECKER_EXE} parse
      ${output_dir}/codechecker.plist
      ${CODECHECKER_PARSE_OPTS}
      || ${CMAKE_COMMAND} -E true # parse has exit code 2 if a report is emitted by an analyzer
    VERBATIM
    USES_TERMINAL
    COMMAND_EXPAND_LISTS
  )
endif()

if(CODECHECKER_STORE OR CODECHECKER_STORE_OPTS)
  add_custom_command(
    TARGET codechecker POST_BUILD
    COMMAND ${CODECHECKER_EXE} store
      ${CODECHECKER_STORE_OPTS}
      ${output_dir}/codechecker.plist
    VERBATIM
    USES_TERMINAL
    COMMAND_EXPAND_LISTS
  )
endif()

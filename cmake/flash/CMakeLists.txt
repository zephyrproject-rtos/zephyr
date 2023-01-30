# SPDX-License-Identifier: Apache-2.0

function(runners_yaml_append content)
  # Append ${content}\n to a target property which is later evaluated as a
  # generator expression when writing the flash runner yaml file.
  # We define this function here to have access to the `flash` target.

  set_property(
    TARGET         runners_yaml_props_target
    APPEND_STRING
    PROPERTY       yaml_contents
    "${content}\n"
    )
endfunction()

function(get_runners_prop prop out_var default_value)
  # Get property 'prop' from runners_yaml_props_target, storing its
  # value in 'out_var'. If the property is not found (value is
  # ...-NOTFOUND), 'out_var' is set to 'default_value'.

  get_target_property(out runners_yaml_props_target "${prop}")

  if("${out}" STREQUAL "out-NOTFOUND")
    set("${out_var}" "${default_value}" PARENT_SCOPE)
  else()
    set("${out_var}" "${out}" PARENT_SCOPE)
  endif()
endfunction()

function(runners_yaml_append_config)
  # Append the common configuration values to the relevant property target.

  runners_yaml_append("\n# Common runner configuration values.")
  runners_yaml_append("config:")
  runners_yaml_append("  board_dir: ${BOARD_DIR}")
  get_runners_prop(elf_file elf "${KERNEL_ELF_NAME}")
  runners_yaml_append("  # Build outputs:")
  runners_yaml_append("  elf_file: ${elf}")
  if(CONFIG_BUILD_OUTPUT_HEX)
    get_runners_prop(hex_file hex "${KERNEL_HEX_NAME}")
  else()
    get_runners_prop(hex_file hex "")
  endif()
  if(hex)
    runners_yaml_append("  hex_file: ${hex}")
  endif()
  if(CONFIG_BUILD_OUTPUT_BIN)
    get_runners_prop(bin_file bin "${KERNEL_BIN_NAME}")
    runners_yaml_append("  bin_file: ${bin}")
  endif()
  if(CONFIG_BUILD_OUTPUT_UF2)
    get_runners_prop(uf2_file uf2 "${KERNEL_UF2_NAME}")
    runners_yaml_append("  uf2_file: ${uf2}")
  endif()

  if(CMAKE_GDB OR OPENOCD OR OPENOCD_DEFAULT_PATH)
    runners_yaml_append("  # Host tools:")
  endif()
  if(CMAKE_GDB)
    runners_yaml_append("  gdb: ${CMAKE_GDB}")
  endif()
  if(OPENOCD)
    runners_yaml_append("  openocd: ${OPENOCD}")
    runners_yaml_append("  openocd_search:")
    if(OPENOCD_DEFAULT_PATH)
      runners_yaml_append("    - ${OPENOCD_DEFAULT_PATH}")
    endif()
  endif()
  runners_yaml_append("")
endfunction()

# Save runner state in a YAML file, and put that YAML file's location
# in the cache.
function(create_runners_yaml)
  set(runners ${ARGV})

  set(runners_yaml "${PROJECT_BINARY_DIR}/runners.yaml")

  runners_yaml_append("# Available runners configured by board.cmake.\nrunners:")
  foreach(runner ${runners})
    runners_yaml_append("- ${runner}")
  endforeach()

  if(DEFINED BOARD_FLASH_RUNNER)
    runners_yaml_append("\n# Default flash runner if --runner is not given.")
    runners_yaml_append("flash-runner: ${BOARD_FLASH_RUNNER}")
  endif()
  if(DEFINED BOARD_DEBUG_RUNNER)
    runners_yaml_append("\n# Default debug runner if --runner is not given.")
    runners_yaml_append("debug-runner: ${BOARD_DEBUG_RUNNER}")
  endif()

  # Sets up common runner configuration values.
  runners_yaml_append_config()

  # Get runner-specific arguments set in the board files.
  runners_yaml_append("# Runner specific arguments")
  runners_yaml_append("args:")
  foreach(runner ${runners})
    string(MAKE_C_IDENTIFIER ${runner} runner_id)
    runners_yaml_append("  ${runner}:")
    get_property(args GLOBAL PROPERTY "BOARD_RUNNER_ARGS_${runner_id}")
    if(args)
      # Usually, the runner has arguments. Append them to runners.yaml,
      # one per line.
      foreach(arg ${args})
        runners_yaml_append("    - ${arg}")
      endforeach()
    else()
      # If the runner doesn't need any arguments, just use an empty list.
      runners_yaml_append("    []\n")
    endif()
  endforeach()

  # Write the final contents and set its location in the cache.
  file(GENERATE OUTPUT "${runners_yaml}" CONTENT
    $<TARGET_PROPERTY:runners_yaml_props_target,yaml_contents>)
  set(ZEPHYR_RUNNERS_YAML "${runners_yaml}" CACHE INTERNAL
    "a configuration file for the runners Python package")
endfunction()

get_property(RUNNERS GLOBAL PROPERTY ZEPHYR_RUNNERS)

# Persist the runner-related state.
#
# Available runners and their arguments are configured in board.cmake
# files.
#
# Everything is marked with FORCE so that re-running CMake updates the
# configuration if the board files change.
if(RUNNERS)
  create_runners_yaml(${RUNNERS})
endif()

zephyr_get(WEST_DIR)
if(WEST_DIR)
  set(WEST "PYTHONPATH=${WEST_DIR}/src" "${PYTHON_EXECUTABLE};${WEST_DIR}/src/west/app/main.py;--zephyr-base=${ZEPHYR_BASE} ")
endif()

# Generate the flash, debug, debugserver, attach targets within the build
# system itself.
foreach(target flash debug debugserver attach)
  if(target STREQUAL flash)
    set(comment "Flashing ${BOARD}")
  elseif(target STREQUAL debug)
    set(comment "Debugging ${BOARD}")
  elseif(target STREQUAL debugserver)
    set(comment "Debugging ${BOARD}")
    if(SUPPORTED_EMU_PLATFORMS)
      # cmake/qemu/CMakeLists.txt will add a debugserver target for
      # emulation platforms, so we don't add one here
      continue()
    endif()
  elseif(target STREQUAL attach)
    set(comment "Debugging ${BOARD}")
  endif()
  string(TOUPPER ${target} TARGET_UPPER)

  list(APPEND RUNNERS_DEPS ${logical_target_for_zephyr_elf})

  # Enable verbose output, if requested.
  if(CMAKE_VERBOSE_MAKEFILE)
    set(RUNNER_VERBOSE "--verbose")
  else()
    set(RUNNER_VERBOSE)
  endif()

  if(WEST)
    add_custom_target(${target}
      # This script will print an error message and fail if <target> has added
      # dependencies. This is done using dedicated CMake script, as
      # `cmake -E {true|false}` is not available until CMake 3.16.
      COMMAND ${CMAKE_COMMAND}
        -DTARGET=${target}
        -DDEPENDENCIES="$<TARGET_PROPERTY:${target},MANUALLY_ADDED_DEPENDENCIES>"
        -P ${CMAKE_CURRENT_LIST_DIR}/check_runner_dependencies.cmake
      COMMAND
        ${CMAKE_COMMAND} -E env
        ${WEST}
        ${RUNNER_VERBOSE}
        ${target}
      WORKING_DIRECTORY
        ${APPLICATION_BINARY_DIR}
      COMMENT
        ${comment}
      USES_TERMINAL
    )
  else()
    add_custom_target(${target}
      COMMAND ${CMAKE_COMMAND} -E echo \"West was not found in path. To support
          '${CMAKE_MAKE_PROGRAM} ${target}', please create a west workspace.\"
      USES_TERMINAL
      )
  endif(WEST)
endforeach()

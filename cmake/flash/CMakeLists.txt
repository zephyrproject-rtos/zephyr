# SPDX-License-Identifier: Apache-2.0

add_custom_target(runner_yml_props_target)

function(runner_yml_write content)
  # Append ${content}\n to a target property which is later evaluated as a
  # generator expression when writing the flash runner yaml file.
  # We define this function here to have access to the `flash` target.

  set_property(
    TARGET         runner_yml_props_target
    APPEND_STRING
    PROPERTY       yaml_contents
    "${content}\n"
    )
endfunction()


# Save runner state in a YAML file, and put that YAML file's location
# in the cache.
function(create_runners_yaml)
  set(runners ${ARGV})

  set(runners_yaml "${PROJECT_BINARY_DIR}/runners.yaml")

  runner_yml_write("# Available runners configured by board.cmake.\nrunners:")
  foreach(runner ${runners})
    runner_yml_write("- ${runner}")
  endforeach()

  if(DEFINED BOARD_FLASH_RUNNER)
    runner_yml_write("\n# Default flash runner if --runner is not given.")
    runner_yml_write("flash-runner: ${BOARD_FLASH_RUNNER}")
  endif()
  if(DEFINED BOARD_DEBUG_RUNNER)
    runner_yml_write("\n# Default debug runner if --runner is not given.")
    runner_yml_write("debug-runner: ${BOARD_DEBUG_RUNNER}")
  endif()

  runner_yml_write("\n# Default command line arguments. The ones in \"common\" are always given.\n# The other sub-keys give runner-specific arguments.")
  runner_yml_write("args:\n  common:")

  # Get default settings for common arguments.
  #
  # TODO: clean up the host tools arguments. These are really runner
  # specific; they only continue to exist here out of inertia.

  runner_yml_write("\
  - --board-dir=${BOARD_DIR}
  - --elf-file=${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}
  - --hex-file=${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}
  - --bin-file=${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}")
  if (DEFINED CMAKE_GDB)
    runner_yml_write("  - --gdb=${CMAKE_GDB}")
  endif()
  if (DEFINED OPENOCD)
    runner_yml_write("  - --openocd=${OPENOCD}")
  endif()
  if(DEFINED OPENOCD_DEFAULT_PATH)
    runner_yml_write("  - --openocd-search=${OPENOCD_DEFAULT_PATH}")
  endif()

  # Get runner-specific arguments set in the board files.
  foreach(runner ${runners})
    string(MAKE_C_IDENTIFIER ${runner} runner_id)
    runner_yml_write("  ${runner}:")
    get_property(args GLOBAL PROPERTY "BOARD_RUNNER_ARGS_${runner_id}")
    if(args)
      # Usually, the runner has arguments. Append them to runners.yaml,
      # one per line.
      foreach(arg ${args})
        runner_yml_write("    - ${arg}")
      endforeach()
    else()
      # If the runner doesn't need any arguments, just use an empty list.
      runner_yml_write("    []\n")
    endif()
  endforeach()

  # Write the final contents and set its location in the cache.
  file(GENERATE OUTPUT "${runners_yaml}" CONTENT
    $<TARGET_PROPERTY:runner_yml_props_target,yaml_contents>)
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

  # Runner-specific command line arguments obtained from the board's
  # build scripts, the application's scripts, etc.
  foreach(runner ${RUNNERS})
    string(MAKE_C_IDENTIFIER ${runner} runner_id)
    # E.g. args = BOARD_RUNNER_ARGS_openocd, BOARD_RUNNER_ARGS_dfu_util, etc.
    get_property(runner_args GLOBAL PROPERTY "BOARD_RUNNER_ARGS_${runner_id}")
    set(ZEPHYR_RUNNER_ARGS_${runner_id} ${runner_args} CACHE STRING
      "Runner-specific arguments for ${runner}" FORCE)
  endforeach()
endif()

if(DEFINED ENV{WEST_DIR} AND NOT WEST_DIR)
  set(WEST_DIR $ENV{WEST_DIR})
endif(DEFINED ENV{WEST_DIR} AND NOT WEST_DIR)

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
    if(EMU_PLATFORM)
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

  # We pass --skip-rebuild here because the DEPENDS value ensures the
  # build is already up to date before west is run.
  if(WEST)
    set(cmd
      ${CMAKE_COMMAND} -E env
      ${WEST}
      ${RUNNER_VERBOSE}
      ${target}
      --skip-rebuild
      DEPENDS ${RUNNERS_DEPS}
              $<TARGET_PROPERTY:zephyr_property_target,${TARGET_UPPER}_DEPENDENCIES>
      WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
      )

    add_custom_target(${target}
      COMMAND
      ${cmd}
      COMMENT
      ${comment}
      USES_TERMINAL
      )

    # This is an artificial target to allow west to ensure all dependencies of
    # target is build before carrying out the actual command.
    # This allows `west` to run just the dependencies before flashing.
    add_custom_target(west_${target}_depends
      COMMAND ${CMAKE_COMMAND} -E echo ""
      DEPENDS ${RUNNERS_DEPS}
              $<TARGET_PROPERTY:zephyr_property_target,${TARGET_UPPER}_DEPENDENCIES>
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

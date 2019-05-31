# SPDX-License-Identifier: Apache-2.0

get_property(RUNNERS GLOBAL PROPERTY ZEPHYR_RUNNERS)

# Enable verbose output, if requested.
if(CMAKE_VERBOSE_MAKEFILE)
  set(RUNNER_VERBOSE "--verbose")
else()
  set(RUNNER_VERBOSE)
endif()

# Persist the runner-related state in the cache.  Everything except
# the list of available runners is configurable, but the set of
# pre-configured runners is internal, since they must be configured through
# the board files.
#
# Everything is marked with FORCE so that re-running CMake updates the
# configuration if the board files change.
if(RUNNERS)
  set(ZEPHYR_RUNNERS ${RUNNERS} CACHE INTERNAL "Available runners")

  # Runner configuration. This is provided to all runners, and is
  # distinct from the free-form arguments provided by e.g.
  # board_runner_args().
  #
  # Always applicable:
  set(ZEPHYR_RUNNER_CONFIG_BOARD_DIR "${BOARD_DIR}"
    CACHE STRING "Board definition directory" FORCE)
  set(ZEPHYR_RUNNER_CONFIG_KERNEL_ELF "${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}"
    CACHE STRING "Path to kernel image in ELF format" FORCE)
  get_property(HEX_FILES_TO_MERGE GLOBAL PROPERTY HEX_FILES_TO_MERGE)
  if(HEX_FILES_TO_MERGE)
    set(ZEPHYR_RUNNER_CONFIG_KERNEL_HEX "${PROJECT_BINARY_DIR}/${MERGED_HEX_NAME}"
      CACHE STRING "Path to merged image in Intel Hex format" FORCE)
  else()
    set(ZEPHYR_RUNNER_CONFIG_KERNEL_HEX "${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}"
      CACHE STRING "Path to kernel image in Intel Hex format" FORCE)
  endif()
  set(ZEPHYR_RUNNER_CONFIG_KERNEL_BIN "${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}"
    CACHE STRING "Path to kernel image as raw binary" FORCE)
  # Not always applicable, but so often needed that they're provided
  # by default: (TODO: clean this up)
  if(DEFINED CMAKE_GDB)
    set(ZEPHYR_RUNNER_CONFIG_GDB "${CMAKE_GDB}"
      CACHE STRING "Path to GDB binary, if applicable" FORCE)
  endif()
  if(DEFINED OPENOCD)
    set(ZEPHYR_RUNNER_CONFIG_OPENOCD "${OPENOCD}"
      CACHE STRING "Path to openocd binary, if applicable" FORCE)
  endif()
  if(DEFINED OPENOCD_DEFAULT_PATH)
    set(ZEPHYR_RUNNER_CONFIG_OPENOCD_SEARCH "${OPENOCD_DEFAULT_PATH}"
      CACHE STRING "Path to add to openocd search path, if applicable" FORCE)
  endif()

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
if(BOARD_FLASH_RUNNER)
  set(ZEPHYR_BOARD_FLASH_RUNNER ${BOARD_FLASH_RUNNER} CACHE STRING
    "Default runner for flashing binaries" FORCE)
endif()
if(BOARD_DEBUG_RUNNER)
  set(ZEPHYR_BOARD_DEBUG_RUNNER ${BOARD_DEBUG_RUNNER} CACHE STRING
    "Default runner for debugging" FORCE)
endif()

if(DEFINED ENV{WEST_DIR} AND NOT WEST_DIR)
  set(WEST_DIR $ENV{WEST_DIR})
endif(DEFINED ENV{WEST_DIR} AND NOT WEST_DIR)

if(WEST_DIR)
  set(WEST "PYTHONPATH=${WEST_DIR}/src" "${PYTHON_EXECUTABLE};${WEST_DIR}/src/west/main.py;--zephyr-base=$ENV{ZEPHYR_BASE} ")
endif()

# Generate the flash, debug, debugserver, attach targets within the build
# system itself.
if(WEST)
  # Verify that the west version found can be used for flashing, e.g. is not the bootstrapper.
  execute_process(COMMAND ${WEST} flash -h
                  RESULT_VARIABLE return_code
                  OUTPUT_QUIET
                  ERROR_QUIET)
  if(NOT ${return_code})
    set(WEST_FLASH True)
  endif(NOT ${return_code})
endif(WEST)

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

  list(APPEND FLASH_DEPS ${logical_target_for_zephyr_elf})

  # We pass --skip-rebuild here because the DEPENDS value ensures the
  # build is already up to date before west is run.
  if(WEST_FLASH)
    set(cmd
      ${CMAKE_COMMAND} -E env
      ${WEST}
      ${RUNNER_VERBOSE}
      ${target}
      --skip-rebuild
      DEPENDS ${FLASH_DEPS}
      WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
      )

    add_custom_target(${target}
      COMMAND
      ${cmd}
      COMMENT
      ${comment}
      USES_TERMINAL
      )
  elseif(WEST)
    add_custom_target(${target}
      COMMAND ${CMAKE_COMMAND} -E echo \"West version found in path does not support
          '${CMAKE_MAKE_PROGRAM} ${target}', ensure west is installed and not only
          the bootstrapper. run 'west init' to fetch west.\"
      USES_TERMINAL
      )
  else()
    add_custom_target(${target}
      COMMAND ${CMAKE_COMMAND} -E echo \"West was not found in path. To support
          '${CMAKE_MAKE_PROGRAM} ${target}', please install west bootstrapper with:
          'pip install west --user', and thereafter 'west init'.\"
      USES_TERMINAL
      )
  endif(WEST_FLASH)
endforeach()

assert_not(FLASH_SCRIPT "FLASH_SCRIPT has been removed; use BOARD_FLASH_RUNNER")
assert_not(DEBUG_SCRIPT "DEBUG_SCRIPT has been removed; use BOARD_DEBUG_RUNNER")

get_property(RUNNERS GLOBAL PROPERTY ZEPHYR_RUNNERS)

# These arguments are common to all runners.
set(RUNNER_ARGS_COMMON
  # Required:
  "--board-dir=${BOARD_DIR}"
  "--kernel-elf=${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}"
  "--kernel-hex=${PROJECT_BINARY_DIR}/${KERNEL_HEX_NAME}"
  "--kernel-bin=${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}"
  # Optional, but so often needed that they're provided by default:
  # (TODO: revisit whether we really want these here)
  "--gdb=${CMAKE_GDB}"
  "--openocd=${OPENOCD}"
  "--openocd-search=${OPENOCD_DEFAULT_PATH}"
  )

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
  set(ZEPHYR_RUNNER_ARGS_COMMON ${RUNNER_ARGS_COMMON} CACHE STRING
    "Common arguments to all runners" FORCE)
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

# Generate the flash, debug, debugserver targets within the build
# system itself.
foreach(target flash debug debugserver)
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
  endif()

  # We pass --skip-rebuild here because the DEPENDS value ensures the
  # build is already up to date before west is run.
  set(cmd
    ${CMAKE_COMMAND} -E env
    PYTHONPATH=${ZEPHYR_BASE}/scripts/meta
    ${PYTHON_EXECUTABLE}
    -m west
    ${RUNNER_VERBOSE}
    ${target}
    --skip-rebuild
    DEPENDS ${logical_target_for_zephyr_elf}
    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    )

  add_custom_target(${target}
    COMMAND
    ${cmd}
    COMMENT
    ${comment}
    USES_TERMINAL
    )
endforeach()

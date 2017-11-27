assert_not(FLASH_SCRIPT "FLASH_SCRIPT has been removed; use BOARD_FLASH_RUNNER")
assert_not(DEBUG_SCRIPT "DEBUG_SCRIPT has been removed; use BOARD_DEBUG_RUNNER")

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

foreach(target flash debug debugserver)
  string(TOUPPER "${target}" target_upper)

  if(target STREQUAL flash)
    set(comment "Flashing ${BOARD}")
    set(runner "${BOARD_FLASH_RUNNER}")
  elseif(target STREQUAL debug)
    set(comment "Debugging ${BOARD}")
    set(runner "${BOARD_DEBUG_RUNNER}")
  elseif(target STREQUAL debugserver)
    set(comment "Debugging ${BOARD}")
    set(runner "${BOARD_DEBUG_RUNNER}")
    if(EMU_PLATFORM)
      # cmake/qemu/CMakeLists.txt will add a debugserver target for
      # emulation platforms, so we don't add one here
      continue()
    endif()
  endif()

  if(runner)
    # E.g. runner_ident = dfu_util (note the underscore), etc.
    string(MAKE_C_IDENTIFIER "${runner}" runner_ident)
    # E.g. args = BOARD_RUNNER_ARGS_openocd, BOARD_RUNNER_ARGS_dfu_util, etc.
    get_property(args GLOBAL PROPERTY "BOARD_RUNNER_ARGS_${runner_ident}")
    set(cmd
      ${CMAKE_COMMAND} -E env
      ${PYTHON_EXECUTABLE}
      $ENV{ZEPHYR_BASE}/scripts/support/zephyr_flash_debug.py
      ${RUNNER_VERBOSE}
      ${runner}
      ${target}
      ${RUNNER_ARGS_COMMON}
      ${args}
      DEPENDS ${logical_target_for_zephyr_elf}
      WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
      )
  else()
    set(cmd
      ${CMAKE_COMMAND} -E echo
	  "'${target}' is not supported with this board."
	  "Please check the documentation for alternate instructions."
      )
  endif()

  add_custom_target(${target}
    COMMAND
    ${cmd}
    COMMENT
    ${comment}
    )
endforeach()

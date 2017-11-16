assert_not(FLASH_SCRIPT "FLASH_SCRIPT has been removed; use BOARD_FLASH_RUNNER")
assert_not(DEBUG_SCRIPT "DEBUG_SCRIPT has been removed; use BOARD_DEBUG_RUNNER")

get_property(ENV_VARS GLOBAL PROPERTY FLASH_SCRIPT_ENV_VARS)

set(ENV_VARS_FORMATTED "")
foreach(env_var ${ENV_VARS})
  list(APPEND ENV_VARS_FORMATTED
    ${env_var}=${${env_var}}
    )
endforeach()

list(APPEND ENV_VARS_FORMATTED
                O=${PROJECT_BINARY_DIR}
  KERNEL_ELF_NAME=${KERNEL_ELF_NAME}
  KERNEL_HEX_NAME=${KERNEL_HEX_NAME}
  KERNEL_BIN_NAME=${KERNEL_BIN_NAME}
        BOARD_DIR=${BOARD_DIR}
              GDB=${CMAKE_GDB}
          OPENOCD_DEFAULT_PATH=${OPENOCD_DEFAULT_PATH}
          OPENOCD=${OPENOCD}
  )

foreach(target flash debug debugserver)
  if(target STREQUAL flash)
    set(comment "Flashing ${BOARD}")
    set(runner ${BOARD_FLASH_RUNNER})
  elseif(target STREQUAL debug)
    set(comment "Debugging ${BOARD}")
    set(runner ${BOARD_DEBUG_RUNNER})
  elseif(target STREQUAL debugserver)
    set(comment "Debugging ${BOARD}")
    set(runner ${BOARD_DEBUG_RUNNER})
    if(EMU_PLATFORM)
      # cmake/qemu/CMakeLists.txt will add a debugserver target for
      # emulation platforms, so we don't add one here
      continue()
    endif()
  endif()

  if(runner)
    set(cmd
      ${CMAKE_COMMAND} -E env
      ${ENV_VARS_FORMATTED}
      ${PYTHON_EXECUTABLE}
      $ENV{ZEPHYR_BASE}/scripts/support/zephyr_flash_debug.py
      ${runner}
      ${target}
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

get_property(ENV_VARS GLOBAL PROPERTY FLASH_SCRIPT_ENV_VARS)
list(APPEND ENV_VARS
                O=${PROJECT_BINARY_DIR}
  KERNEL_ELF_NAME=${KERNEL_ELF_NAME}
  KERNEL_HEX_NAME=${KERNEL_HEX_NAME}
  KERNEL_BIN_NAME=${KERNEL_BIN_NAME}
             ARCH=${ARCH}
       BOARD_NAME=${BOARD}
  )

foreach(target flash debug debugserver)
  if(target STREQUAL flash)
    set(comment "Flashing ${BOARD}")
    set(script ${FLASH_SCRIPT})
  elseif(target STREQUAL debug)
    set(comment "Debugging ${BOARD}")
    set(script ${DEBUG_SCRIPT})
  elseif(target STREQUAL debugserver)
    set(comment "Debugging ${BOARD}")
    set(script ${DEBUG_SCRIPT})
    if(EMU_PLATFORM)
      # cmake/qemu/CMakeLists.txt will add a debugserver target for
      # emulation platforms, so we don't add one here
      continue()
    endif()
  endif()

  if(script)
    set(cmd
      ${CMAKE_COMMAND} -E env
      ${ENV_VARS}

      ${CONFIG_SHELL}
      $ENV{ZEPHYR_BASE}/scripts/support/${script}
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

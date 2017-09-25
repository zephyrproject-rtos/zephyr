get_property(ENV_VARS GLOBAL PROPERTY FLASH_SCRIPT_ENV_VARS)
list(APPEND ENV_VARS
  O=${PROJECT_BINARY_DIR}
  KERNEL_ELF_NAME=${KERNEL_ELF_NAME}
  KERNEL_HEX_NAME=${KERNEL_HEX_NAME}
  KERNEL_BIN_NAME=${KERNEL_BIN_NAME}
  )

if(FLASH_SCRIPT)
  add_custom_target(flash
    COMMAND

    ${CMAKE_COMMAND} -E env
    ${ENV_VARS}

    ${CONFIG_SHELL}
    $ENV{ZEPHYR_BASE}/scripts/support/${FLASH_SCRIPT}
    flash

    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    COMMENT "Flashing ${BOARD}"
    )
else()
  add_custom_target(flash
    COMMAND
    ${CMAKE_COMMAND} -E echo
	"Flashing not supported with this board."
	"Please check the documentation for alternate instructions."
    COMMENT "Flashing ${BOARD}"
    )
endif()

if(DEBUG_SCRIPT)
  add_custom_target(debug
    COMMAND

    ${CMAKE_COMMAND} -E env
    ${ENV_VARS}

    ${CONFIG_SHELL}
    $ENV{ZEPHYR_BASE}/scripts/support/${DEBUG_SCRIPT}
    debug

    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    )
else()
  add_custom_target(debug
    COMMAND
    ${CMAKE_COMMAND} -E echo
	"Debugging not supported with this board."
	"Please check the documentation for alternate instructions."
    )
endif()

if(NOT EMU_PLATFORM)
  if(DEBUG_SCRIPT)
    add_custom_target(debugserver
      COMMAND

      ${CMAKE_COMMAND} -E env
      ${ENV_VARS}

      ${CONFIG_SHELL}
      $ENV{ZEPHYR_BASE}/scripts/support/${DEBUG_SCRIPT}
      debugserver

      WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
      )
  else()
    add_custom_target(debugserver
      COMMAND
      ${CMAKE_COMMAND} -E echo
	  "Debugserver not supported with this board."
	  "Please check the documentation for alternate instructions."
      )
  endif()
endif()

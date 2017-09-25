if(FLASH_SCRIPT)
  get_property(ENV_VARS GLOBAL PROPERTY FLASH_SCRIPT_ENV_VARS)

  add_custom_target(flash
    COMMAND

    ${CMAKE_COMMAND} -E env
    O=${PROJECT_BINARY_DIR}
    KERNEL_ELF_NAME=${KERNEL_ELF_NAME}
    KERNEL_HEX_NAME=${KERNEL_HEX_NAME}
    KERNEL_BIN_NAME=${KERNEL_BIN_NAME}
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


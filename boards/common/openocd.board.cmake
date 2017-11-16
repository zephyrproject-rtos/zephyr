set_ifndef(BOARD_FLASH_RUNNER openocd)
set_ifndef(BOARD_DEBUG_RUNNER openocd)

# "load_image" or "flash write_image erase"?
if(CONFIG_X86 OR CONFIG_ARC)
  set_ifndef(OPENOCD_USE_LOAD_IMAGE YES)
endif()
if(OPENOCD_USE_LOAD_IMAGE)
  set_ifndef(OPENOCD_FLASH load_image)
else()
  set_ifndef(OPENOCD_FLASH flash write_image erase)
endif()

# zephyr.bin, or something else?
set_ifndef(OPENOCD_IMAGE "${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}")

# CONFIG_FLASH_BASE_ADDRESS, or something else?
if(NOT DEFINED OPENOCD_ADDRESS)
  # This can't use set_ifndef() because CONFIG_FLASH_BASE_ADDRESS is
  # the empty string on some targets, which causes set_ifndef() to
  # choke.
  set(OPENOCD_ADDRESS "${CONFIG_FLASH_BASE_ADDRESS}")
endif()

set(OPENOCD_CMD_LOAD_DEFAULT ${OPENOCD_FLASH} ${OPENOCD_IMAGE} ${OPENOCD_ADDRESS})
set(OPENOCD_CMD_VERIFY_DEFAULT verify_image ${OPENOCD_IMAGE} ${OPENOCD_ADDRESS})

board_finalize_runner_args(openocd
  "--cmd-load=\"${OPENOCD_CMD_LOAD_DEFAULT}\""
  "--cmd-verify=\"${OPENOCD_CMD_VERIFY_DEFAULT}\""
  )

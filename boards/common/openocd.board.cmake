# SPDX-License-Identifier: Apache-2.0

set_ifndef(BOARD_FLASH_RUNNER openocd)
set_ifndef(BOARD_DEBUG_RUNNER openocd)

# "load_image" or "flash write_image erase"?
if(CONFIG_X86 OR CONFIG_ARC)
  set_ifndef(OPENOCD_USE_LOAD_IMAGE YES)
endif()
if(OPENOCD_USE_LOAD_IMAGE)
  set_ifndef(OPENOCD_FLASH load_image)
else()
  set_ifndef(OPENOCD_FLASH "flash write_image erase")
endif()

# zephyr.elf, or something else?
set_ifndef(OPENOCD_IMAGE "${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}")

set(OPENOCD_CMD_LOAD_DEFAULT "${OPENOCD_FLASH} ${OPENOCD_IMAGE}")
set(OPENOCD_CMD_VERIFY_DEFAULT "verify_image ${OPENOCD_IMAGE}")

board_finalize_runner_args(openocd
  --cmd-load "${OPENOCD_CMD_LOAD_DEFAULT}"
  --cmd-verify "${OPENOCD_CMD_VERIFY_DEFAULT}"
  )

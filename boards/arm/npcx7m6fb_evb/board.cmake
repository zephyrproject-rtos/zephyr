# SPDX-License-Identifier: Apache-2.0

set(TARGET_IMAGE_FILE  ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}_${BOARD}.bin)

set(TARGET_IMAGE_ADDR  ${CONFIG_FLASH_BASE_ADDRESS})
set(TARGET_IMAGE_SIZE  ${CONFIG_FLASH_SIZE})

board_set_flasher_ifnset(openocd)
board_set_debugger_ifnset(openocd)

board_finalize_runner_args(openocd
  --cmd-load "flash_npcx ${MONITOR_IMAGE_FILE} ${TARGET_IMAGE_FILE} ${TARGET_IMAGE_ADDR} ${TARGET_IMAGE_SIZE}"
  --cmd-verify "verify_npcx"
  )

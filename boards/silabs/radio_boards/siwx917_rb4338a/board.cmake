# Copyright (c) 2024 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0
# Set the flash file based on signing is enabled or not.

if(CONFIG_SIWX91X_SILABS_OTA_SIGN_ENABLE OR CONFIG_SIWX91X_SILABS_OTA_MIC_ENABLE OR CONFIG_SIWX91X_SILABS_OTA_ENCRYPT_ENABLE)
    set(FLASH_FILE ${PROJECT_BINARY_DIR}/${KERNEL_NAME}_secure.bin.rps)
else()
    set(FLASH_FILE ${PROJECT_BINARY_DIR}/${KERNEL_BIN_NAME}.rps)
endif()
board_runner_args(silabs_commander "--device=SiWG917M111GTBA" "--file-type=bin"
    "--file=${FLASH_FILE}")
include(${ZEPHYR_BASE}/boards/common/silabs_commander.board.cmake)

# It is not possible to load/flash a firmware using JLink, but it is possible to
# debug a firmware with:
#    west attach -r jlink
# Once started, it should be possible to reset the device with "monitor reset"
board_runner_args(jlink "--device=Si917" "--speed=10000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
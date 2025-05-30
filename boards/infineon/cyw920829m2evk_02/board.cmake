# Copyright (c) 2024 Cypress Semiconductor Corporation.
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--target-handle=TARGET.cm33")

# MCUboot requires a flashloader with 64k erase size, please use 'west blobs fetch hal_infineon' to download it.
if(CONFIG_BOOTLOADER_MCUBOOT)
  set(flashloader_blobs_path ${ZEPHYR_HAL_INFINEON_MODULE_DIR}/zephyr/blobs/flashloader/TARGET_CYW920829M2EVK-02)

  if(NOT EXISTS ${flashloader_blobs_path}/CYW208xx_SMIF_64K.FLM)
    message(WARNING "MCUboot requires a flashloader with 64k erase size, please use 'west blobs fetch hal_infineon' to download it")
  else()
    board_runner_args(openocd "--openocd-search=${flashloader_blobs_path}")
    board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_CYW208xx_SMIF_64K.cfg")
  endif()
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
board_runner_args(jlink "--device=CYW20829_tm")
include (${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

set_property(TARGET runners_yaml_props_target PROPERTY hex_file zephyr_merged.hex)

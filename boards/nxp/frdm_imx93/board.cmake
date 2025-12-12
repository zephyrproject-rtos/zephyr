#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NXP_SPSDK_IMAGE OR (DEFINED ENV{USE_NXP_SPSDK_IMAGE}
  AND "$ENV{USE_NXP_SPSDK_IMAGE}" STREQUAL "y"))
  board_set_flasher_ifnset(spsdk)

  board_runner_args(spsdk "--family=mimx9352")
  # reuse evk board's bootloader
  board_runner_args(spsdk "--bootloader=${CMAKE_BINARY_DIR}/zephyr/imx93evk-boot-firmware-6.12.34-2.1.0/imx-boot-imx93evk-sd.bin-flash_singleboot")
  board_runner_args(spsdk "--flashbin=${CMAKE_BINARY_DIR}/zephyr/flash.bin")
  board_runner_args(spsdk "--containers=one")

  include(${ZEPHYR_BASE}/boards/common/spsdk.board.cmake)
endif()

# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SOF AND CONFIG_BOARD_IMX95_EVK_MIMX9596_M7_DDR)
  board_set_rimage_target(imx95)
endif()

board_set_flasher_ifnset(spsdk)

board_runner_args(spsdk "--bootdevice=sd")
board_runner_args(spsdk "--family=mimx9596")
board_runner_args(spsdk "--bootloader=${ZEPHYR_HAL_NXP_MODULE_DIR}/zephyr/blobs/imx-firmware/imx95-19x19-lpddr5-evk/imx-boot-imx95-19x19-lpddr5-evk-sd.bin-flash_a55")
board_runner_args(spsdk "--flashbin=${CMAKE_BINARY_DIR}/zephyr/flash.bin")

include(${ZEPHYR_BASE}/boards/common/spsdk.board.cmake)

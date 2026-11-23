# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NXP_SPSDK_IMAGE OR (DEFINED ENV{USE_NXP_SPSDK_IMAGE}
  AND "$ENV{USE_NXP_SPSDK_IMAGE}" STREQUAL "y"))
  board_set_flasher_ifnset(spsdk)

  board_runner_args(spsdk "--family=mx952")
  board_runner_args(spsdk "--bootloader=${CMAKE_BINARY_DIR}/zephyr/imx952-evk-boot-firmware-6.18.20-2.0.0/imx-boot-imx952-19x19-lpddr5-evk-sd.bin-flash_all")
  board_runner_args(spsdk "--flashbin=${CMAKE_BINARY_DIR}/zephyr/flash.bin")
  if(CONFIG_CPU_CORTEX_A55)
    board_runner_args(spsdk "--containers=two")
  else()
    board_runner_args(spsdk "--containers=one")
  endif()

  include(${ZEPHYR_BASE}/boards/common/spsdk.board.cmake)
endif()

if(CONFIG_SOC_MIMX9529_A55)
  board_runner_args(jlink "--device=IMX952_A55" "--no-reset" "--flash-sram")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()

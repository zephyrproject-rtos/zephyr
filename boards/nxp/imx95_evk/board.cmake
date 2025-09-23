# SPDX-License-Identifier: Apache-2.0

if (CONFIG_SOF AND CONFIG_BOARD_IMX95_EVK_MIMX9596_M7_DDR)
  board_set_rimage_target(imx95)
endif()

if(CONFIG_BOARD_NXP_SPSDK_IMAGE OR (DEFINED ENV{USE_NXP_SPSDK_IMAGE}
  AND "$ENV{USE_NXP_SPSDK_IMAGE}" STREQUAL "y"))
  board_set_flasher_ifnset(spsdk)

  board_runner_args(spsdk "--family=mimx9596")
  board_runner_args(spsdk "--bootloader=${CMAKE_BINARY_DIR}/zephyr/imx95-19x19-lpddr5-evk-boot-firmware-0.1/imx-boot-imx95-19x19-lpddr5-evk-sd.bin-flash_all")
  board_runner_args(spsdk "--flashbin=${CMAKE_BINARY_DIR}/zephyr/flash.bin")
  if(CONFIG_CPU_CORTEX_A55)
    board_runner_args(spsdk "--containers=two")
  else()
    board_runner_args(spsdk "--containers=one")
  endif()

  include(${ZEPHYR_BASE}/boards/common/spsdk.board.cmake)
endif()

if(CONFIG_SOC_MIMX9596_A55)
  board_runner_args(jlink "--device=MIMX9596_A55_0" "--no-reset" "--flash-sram")
  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
endif()

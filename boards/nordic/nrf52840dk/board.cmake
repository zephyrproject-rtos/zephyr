# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_NRF52840_QIAA)
  board_runner_args(nrfutil "--ext-mem-config-file=${BOARD_DIR}/support/nrf52840dk_qspi_nrfutil_config.json")
endif()

board_runner_args(jlink "--device=nRF52840_xxAA" "--speed=4000")
board_runner_args(pyocd "--target=nrf52840" "--frequency=4000000")
include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-nrf5.board.cmake)

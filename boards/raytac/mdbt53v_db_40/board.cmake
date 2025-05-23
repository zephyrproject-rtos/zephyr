# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_RAYTAC_MDBT53V_DB_40_NRF5340_CPUAPP OR CONFIG_BOARD_RAYTAC_MDBT53V_DB_40_NRF5340_CPUAPP_NS)
  board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=4000")
elseif(BOARD_RAYTAC_MDBT53V_DB_40_NRF5340_CPUNET)
  board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

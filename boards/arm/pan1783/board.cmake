# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_PAN1783_EVB_CPUAPP OR CONFIG_BOARD_PAN1783A_EVB_CPUAPP OR CONFIG_BOARD_PAN1783A_PA_EVB_CPUAPP)
  board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=4000")
endif()

if(CONFIG_BOARD_PAN1783_EVB_CPUNET OR CONFIG_BOARD_PAN1783A_EVB_CPUNET OR CONFIG_BOARD_PAN1783A_PA_EVB_CPUNET)
  board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

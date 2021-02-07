# Copyright (c) 2019 Nordic Semiconductor ASA
# Copyright (c) 2021 Laird Connectivity
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_BL5340_DVK_CPUAPPNS)
  set(TFM_PUBLIC_KEY_FORMAT "full")
endif()

if((CONFIG_BOARD_BL5340_DVK_CPUAPP OR CONFIG_BOARD_BL5340_DVK_CPUAPPNS))
board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=8000")
endif()

if(CONFIG_BOARD_BL5340_DVK_CPUNET)
board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=8000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

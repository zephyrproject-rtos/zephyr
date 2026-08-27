# Copyright (c) 2024 Nordic Semiconductor ASA
# Copyright (c) 2025 Raytac Corporation.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_RAYTAC_AN7002Q_DB_NRF5340_CPUAPP)
  board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=4000")
elseif(CONFIG_BOARD_RAYTAC_AN7002Q_DB_NRF5340_CPUNET)
  board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

# Copyright (c) 2020 Ruuvi Innovations Ltd (Oy)
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=nrf52" "--speed=4000")
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

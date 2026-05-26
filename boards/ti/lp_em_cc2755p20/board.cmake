# Copyright (c) 2026 Texas Instruments Incorporated
# Copyright (c) 2024 BayLibre, SAS
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=CC2755P20")
board_runner_args(openocd --cmd-pre-init "source [find board/ti_lp_em_cc2755P20.cfg]")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/ti_openocd.board.cmake)

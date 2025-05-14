# Copyright (c) 2025 Texas Instruments Incorporated
# Copyright (c) 2024 BayLibre, SAS
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=CC2745R10-Q1")
board_runner_args(openocd --cmd-pre-init "source [find board/ti_lp_em_cc2745r10.cfg]")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/ti_openocd.board.cmake)

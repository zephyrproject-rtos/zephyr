# Copyright (c) 2025 Fabian Pflug
# Copyright (c) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=CC2340R5" "--iface=swd")
board_runner_args(openocd --cmd-pre-init "source [find board/ti_lp_em_cc2340r5.cfg]")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/ti_openocd.board.cmake)

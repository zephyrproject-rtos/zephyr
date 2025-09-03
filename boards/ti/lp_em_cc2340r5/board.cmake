# Copyright (c) 2025 Fabian Pflug
# Copyright (c) 2026 Texas Instruments Incorporated
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=CC2340R5" "--iface=swd")
board_runner_args(openocd --cmd-pre-init "source [find board/ti_lp_em_cc2340r5.cfg]")
set(OPENOCD_BASE $ENV{TI_OPENOCD_INSTALL_DIR}/openocd/bin)
set(OPENOCD ${OPENOCD_BASE}/bin/openocd)
set(OPENOCD_DEFAULT_PATH ${OPENOCD_BASE}/share/openocd/scripts)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

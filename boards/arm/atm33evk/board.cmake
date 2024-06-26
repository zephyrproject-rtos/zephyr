# Copyright (c) Atmosic 2022-2024
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(atmisp "--atm_plat=atmx3" "--openocd_config=${ZEPHYR_BASE}/../modules/hal/atmosic_lib/ATM33xx-5/openocd/atmx3_openocd.cfg" "--gdb_config=${ZEPHYR_BASE}/../modules/hal/atmosic_lib/ATM33xx-5/gdb/paris.gdb")
include(${ZEPHYR_BASE}/boards/common/atmisp.board.cmake)

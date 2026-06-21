# Copyright (c) 2024 RAKwireless Technology Limited <www.rakwireless.com>
# Sercan Erat <sercanerat@gmail.com>
# JaeHwan Jin <jaehwan.jin@rakwireless.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=AMA3B1KK-KBR" "--iface=swd" "--speed=1000")
board_runner_args(pyocd "--target=ama3b1kk_kbr")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)

# Copyright (c) 2024 RAKwireless Technology Co., Ltd. <www.rakwireless.com>
# Sercan Erat <sercanerat@gmail.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=AMA3B1KK-KBR" "--iface=swd" "--speed=1000")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

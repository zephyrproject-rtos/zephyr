# Copyright (c) 2023 Ambiq Micro Inc. <www.ambiq.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=AMAP42KK-KBR" "--iface=swd" "--speed=1000")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

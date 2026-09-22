# Copyright (c) 2026 Patryk Koscik <pkoscik@antmicro.com>
# SPDX-License-Identifier: Apache-2.0

board_runner_args(dfu-util "--pid=1209:70b1" "--alt=0")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)

# SPDX-License-Identifier: Apache-2.0

board_runner_args(dfu-util "--pid=2341:035b" "--alt=0" "--dfuse")

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)

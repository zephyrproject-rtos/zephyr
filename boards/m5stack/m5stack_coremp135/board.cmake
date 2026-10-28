# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

board_runner_args(dfu-util "--pid=0483:df11" "--alt=0" "--detach")
include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)

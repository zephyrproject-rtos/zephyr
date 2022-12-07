#
# Copyright (c) 2022 Renesas Electronics Corporation
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(ezflashcli)
board_runner_args(jlink --device=DA14699)
include(${ZEPHYR_BASE}/boards/common/ezflashcli.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

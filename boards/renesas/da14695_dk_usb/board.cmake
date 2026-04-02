#
# Copyright (c) 2023 Renesas Electronics Corporation and/or its affiliates
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(ezflashcli)
board_runner_args(jlink --device=DA14695)
include(${ZEPHYR_BASE}/boards/common/ezflashcli.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

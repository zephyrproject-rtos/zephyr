# Copyright (c) 2021 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd --cmd-pre-init "source [find interface/ftdi/um232h.cfg]")
board_runner_args(openocd --cmd-pre-init "source [find e203hbird.cfg]")
board_runner_args(openocd --cmd-reset-halt "lichee_tang_reset_halt")
board_runner_args(openocd --cmd-pre-load "lichee_tang_pre_load")
board_runner_args(openocd --cmd-post-verify "lichee_tang_post_verify")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

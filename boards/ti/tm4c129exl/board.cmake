# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
#
# Author: Luciano Carricart <carricartluciano@gmail.com> (TM4C129 support)
# Based on TM4C123 support by Sri Surya <srisurya@linumiz.com>

# TI Tiva C Series EK-TM4C129EXL Crypto Connected LaunchPad board configuration

board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd.cfg")

include("${ZEPHYR_BASE}/boards/common/openocd.board.cmake")

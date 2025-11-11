# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2025 Linumiz GmbH
# Author: Sri Surya  <srisurya@linumiz.com>

board_runner_args(jlink "--device=AMA2B1KK" "--iface=swd" "--speed=4000")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

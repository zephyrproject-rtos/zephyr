#
# Copyright (c) 2021 metraTec GmbH
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=LPC51U68JBD64" "--reset-after-load")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

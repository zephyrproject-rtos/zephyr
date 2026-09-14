# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=MIMXRT1152xxx8B" "--no-reset")
board_runner_args(linkserver "--device=MIMXRT1152xxxxx:FRDM-IMXRT1152")

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

# Copyright 2022-2023 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(nxp_pedbg
  "--soc-name" "S32K148"
)

include(${ZEPHYR_BASE}/boards/common/nxp_pedbg.board.cmake)

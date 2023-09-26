#
# Copyright 2022-2023 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

board_runner_args(jlink "--device=MIMXRT595S_M33" "--reset-after-load")
board_runner_args(linkserver  "--device=MIMXRT595S:EVK-MIMXRT595")
board_runner_args(linkserver  "--override=/device/memory/5/flash-driver=MIMXRT500_SFDP_MXIC_OSPI_S.cfx")
board_runner_args(linkserver  "--override=/device/memory/5/location=0x18000000")



include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)

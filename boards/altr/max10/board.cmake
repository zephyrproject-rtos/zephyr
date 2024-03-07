# SPDX-License-Identifier: Apache-2.0

board_runner_args(nios2 "--cpu-sof=${ZEPHYR_BASE}/soc/nios2/nios2f-zephyr/cpu/ghrd_10m50da.sof")
include(${ZEPHYR_BASE}/boards/common/nios2.board.cmake)

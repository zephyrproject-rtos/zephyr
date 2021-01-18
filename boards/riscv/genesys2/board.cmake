# Copyright (c) 2019 SiFive Inc.
# SPDX-License-Identifier: Apache-2.0
#board_runner_args(jlink "--device=FE310")
#board_runner_args(jlink "--iface=JTAG")
#board_runner_args(jlink "--speed=4000")
#board_runner_args(jlink "--tool-opt=-jtagconf -1,-1")
#board_runner_args(jlink "--tool-opt=-autoconnect 1")
#include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

# SPDX-License-Identifier: Apache-2.0

set(OPENOCD_USE_LOAD_IMAGE NO)

board_runner_args(openocd "--config=${BOARD_DIR}/openocd.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)


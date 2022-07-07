# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS renode)
set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/hifive_unleashed.resc)
set(OPENOCD_USE_LOAD_IMAGE NO)

board_runner_args(openocd "--config=${BOARD_DIR}/support/openocd_hifive_unleashed.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

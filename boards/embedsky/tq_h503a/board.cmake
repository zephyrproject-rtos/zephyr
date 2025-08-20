# Copyright (c) 2025 MA Junyi
# SPDX-License-Identifier: Apache-2.0

set(OPENOCD_ROOT "<path_to_openocd_stm32_repo>")
set(OPENOCD "${OPENOCD_ROOT}/src/openocd" CACHE FILEPATH "" FORCE)
set(OPENOCD_DEFAULT_PATH ${OPENOCD_ROOT}/tcl)

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
# FIXME: official openocd runner not yet available.

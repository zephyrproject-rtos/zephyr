# SPDX-License-Identifier: Apache-2.0

board_set_debugger_ifnset(iar)
board_set_flasher_ifnset(iar)
board_finalize_runner_args(iar)

set(IAR_DEBUG_FILE "${APPLICATION_BINARY_DIR}/launch.json" CACHE FILEPATH
  "Path to launch file for IAR Embedded Workbench. File is created by the iar flash/debug runner")

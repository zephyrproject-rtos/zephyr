# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(nios2)
board_set_debugger_ifnset(nios2)

board_finalize_runner_args(nios2
  # TODO: merge this script into nios2.py
  "--quartus-flash=${ZEPHYR_BASE}/scripts/support/quartus-flash.py"
  )

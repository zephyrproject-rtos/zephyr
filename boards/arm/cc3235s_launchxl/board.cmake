# SPDX-License-Identifier: Apache-2.0

# Only loading to RAM not really flashing. Use TI's ImageCreator for signing and
# flashing images.
board_set_flasher_ifnset(openocd)
board_set_debugger_ifnset(openocd)

board_finalize_runner_args(openocd
  --use-elf
  --cmd-reset-halt "soft_reset_halt"
  )

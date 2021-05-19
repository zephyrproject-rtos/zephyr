# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(openocd)
board_set_debugger_ifnset(openocd)

board_finalize_runner_args(openocd
  --cmd-load "npcx_write_image"
  --cmd-verify "npcx_verify_image"
  )

# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(misc-flasher)
board_finalize_runner_args(misc-flasher)

if (CONFIG_IPC_MAJOR_3)
  board_set_rimage_target(tgl)
elseif(CONFIG_IPC_MAJOR_4)
  board_set_rimage_target(tgl-cavs)
endif()

# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Show the console at the speed the devicetree gives it
dt_chosen(console_uart PROPERTY "zephyr,console")
if(console_uart)
  dt_prop(console_baud PATH ${console_uart} PROPERTY "current-speed")
endif()

board_set_flasher_ifnset(bkflash)
if(console_baud)
  board_finalize_runner_args(bkflash "--monitor-baud=${console_baud}")
else()
  board_finalize_runner_args(bkflash)
endif()

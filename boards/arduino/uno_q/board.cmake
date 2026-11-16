# SPDX-License-Identifier: Apache-2.0

board_runner_args(adb-openocd
  "--cmd-pre-init=reset_config srst_only srst_nogate srst_push_pull connect_assert_srst"
  # Clear DHCSR.C_DEBUGEN left set by OpenOCD so the DWT can raise DebugMonitor.
  "--cmd-post-reset=mww 0xE000EDF0 0xA05F0000"
)
board_runner_args(stm32cubeprogrammer "--erase" "--port=swd" "--reset-mode=hw")

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

board_runner_args(jlink "--device=STM32U585AI" "--reset-after-load")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/adb-openocd.board.cmake)
# FIXME: openocd runner requires use of STMicro openocd fork.
# Check board documentation for more details.
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

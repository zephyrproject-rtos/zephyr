# SPDX-License-Identifier: Apache-2.0

# keep first
if(CONFIG_STM32_MEMMAP)
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(stm32cubeprogrammer "--extload=MX25LM51245G_STM32H573I-DK-RevB-SFIx.stldr")
else()
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
endif()

board_runner_args(pyocd "--target=stm32h573iikx")

board_runner_args(probe_rs "--chip=STM32H573II")

board_runner_args(openocd "--tcl-port=6666")
board_runner_args(openocd --cmd-pre-init "gdb_report_data_abort enable")
board_runner_args(openocd "--no-halt")

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/probe-rs.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
# FIXME: official openocd runner not yet available.

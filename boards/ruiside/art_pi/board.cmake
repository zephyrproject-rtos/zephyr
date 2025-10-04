# SPDX-License-Identifier: Apache-2.0

# keep first
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
# downlodad external loader: https://github.com/RT-Thread-Studio/sdk-bsp-stm32h750-realthread-artpi/tree/master/debug/stldr
board_runner_args(stm32cubeprogrammer "--extload=ART-Pi_W25Q64.stldr")
board_runner_args(openocd --target-handle=_CHIPNAME.cpu0)

# keep first
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-stm32.board.cmake)
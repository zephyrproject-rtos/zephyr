# SPDX-License-Identifier: Apache-2.0

if(CONFIG_STM32_MEMMAP)
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
board_runner_args(stm32cubeprogrammer "--extload=MX25LM51245G_STM32H573I-DK-RevB-SFIx.stldr")
else()
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")
endif()

board_runner_args(pyocd "--target=stm32h573iikx")

include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
# FIXME: openocd runner not yet available.

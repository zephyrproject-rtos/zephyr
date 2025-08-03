# Copyright (c) 2025 Ishraq Ibne Ashraf <ishraq.i.ashraf@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(pyocd "--target=nrf52840" "--frequency=4000000")

set(OPENOCD_NRF5_SUBFAMILY "nrf52")
set(OPENOCD_NRF5_INTERFACE "cmsis-dap")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd-nrf5.board.cmake)

# Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

# Default flashing path: drag-and-drop UF2 to the Adafruit nRF52 bootloader
# pre-installed on the Wio Tracker L1. Enter the bootloader by double-tapping
# RST; the board enumerates as a USB Mass Storage volume that west flash
# copies build/zephyr/zephyr.uf2 onto. The Board-ID below matches the value
# advertised in INFO_UF2.TXT on the bootloader volume so that west flash
# picks the right device when more than one UF2 mount is present.
board_runner_args(uf2 "--board-id=TRACKER L1")
include(${ZEPHYR_BASE}/boards/common/uf2.board.cmake)

# Optional fall-back for users with an external SWD probe wired to the
# board's debug pads (e.g. for re-flashing the bootloader).
board_runner_args(jlink "--device=nRF52840_xxAA" "--speed=4000")
board_runner_args(pyocd "--target=nrf52840" "--frequency=4000000")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

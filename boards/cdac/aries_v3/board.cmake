# Copyright (c) 2026 Anuj Deshpande
# SPDX-License-Identifier: Apache-2.0

# Programs are uploaded to the ARIES v3.0 board over UART using the XMODEM
# protocol. Flashing uses the open-source 'vegadude' utility
# (https://github.com/rnayabed/vegadude); the board must be in UART boot mode
# (BOOT SEL jumper open) and reset before running 'west flash'.
include(${ZEPHYR_BASE}/boards/common/vegadude.board.cmake)

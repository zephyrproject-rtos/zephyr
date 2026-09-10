# SPDX-FileCopyrightText: Copyright (c) 2026 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_XIP)
  board_runner_args(openfpgaloader
    --board tangmega138k
    --external-flash
    --offset=0x600000
    --verify
  )

  include(${ZEPHYR_BASE}/boards/common/openfpgaloader.board.cmake)
  board_runner_args(openocd --no-load)
else()
  # Non-XIP images are loaded directly into RAM by OpenOCD.
  set(OPENOCD_USE_LOAD_IMAGE YES)
  board_runner_args(openocd --file-type=elf)
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

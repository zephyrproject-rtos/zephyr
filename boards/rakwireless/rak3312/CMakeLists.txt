# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 RAKwireless Technology Limited

if(CONFIG_BOARD_RAK3312_ESP32S3_PROCPU)

zephyr_library()
zephyr_library_sources(board.c)

endif()

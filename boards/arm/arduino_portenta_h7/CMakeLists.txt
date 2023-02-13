# Copyright (c) 2022 Benjamin Björnsson <benjamin.bjornsson@gmail.com>.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_ARDUINO_PORTENTA_H7_M7)

zephyr_library()
zephyr_library_sources(board.c)

endif()

# Copyright (c) 2019 Interay Solutions B.V.
# Copyright (c) 2019 Oane Kingma
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_UART_GECKO)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()

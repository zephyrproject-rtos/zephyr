# Copyright (c) 2019 Interay Solutions B.V.
# Copyright (c) 2019 Oane Kingma
# Copyright (c) 2020 Thorvald Natvig
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_ETH_GECKO)
  zephyr_library()
  zephyr_library_sources(board.c)
  zephyr_library_include_directories(${ZEPHYR_BASE}}/drivers)
endif()

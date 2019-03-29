#
# Copyright (c) 2018, UNISOC Incorporated
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_WIFI_UWP)
  zephyr_include_directories(./)

  zephyr_sources(
	cmdevt.c
	txrx.c
	ipc.c
	main.c
	mem.c
    )
endif()

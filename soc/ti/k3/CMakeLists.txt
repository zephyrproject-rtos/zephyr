# Copyright (c) 2024 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

add_subdirectory(./common)

if(CONFIG_SOC_SERIES_AM62LX)
	add_subdirectory(am62lx)
endif()

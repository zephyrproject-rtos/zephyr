# Copyright (c) 2024 Andrew Featherstone
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_sources(
	soc.c
)

zephyr_library_sources_ifdef(CONFIG_RPI_PICO_BINARY_INFO
	binary_info.c
	binary_info_header.S
)

zephyr_linker_sources_ifdef(CONFIG_RPI_PICO_BINARY_INFO
	ROM_START SORT_KEY 0x0binary_info_header binary_info_header.ld
)

zephyr_linker_sources_ifdef(CONFIG_RPI_PICO_BINARY_INFO
	RODATA binary_info.ld
)

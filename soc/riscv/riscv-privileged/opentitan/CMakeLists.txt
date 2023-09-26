# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c rom_header.S)

zephyr_linker_sources(ROM_START SORT_KEY 000romheader rom_header.ld)

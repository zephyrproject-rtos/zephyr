# Copyright (c) 2024 Texas Instruments Incorporated
# Copyright (c) 2024 BayLibre, SAS
#
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc.c)
zephyr_sources(ccfg.c)
zephyr_include_directories(.)

zephyr_linker_sources_ifdef(CONFIG_HAS_TI_CCFG SECTIONS ccfg.ld)

SET(crc_tool_search_paths
	/usr
	/usr/local
	/opt
	$ENV{HOME}
	$ENV{HOME}/.local/bin
	$ENV{HOME}/.local/opt
	$ENV{HOME}/bin)

find_program(CRC_TOOL crc_tool PATHS ${crc_tool_search_paths})

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  # Insert CRC32 in CCFG
  COMMAND ${CRC_TOOL} patch-image
	  --elf ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf
	  --symbol-prefix CRC_CCFG
	  --output ${CMAKE_BINARY_DIR}/zephyr/zephyr.elf
  )

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

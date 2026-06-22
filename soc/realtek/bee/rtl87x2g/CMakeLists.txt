# Copyright (c) 2026, Realtek Semiconductor Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library_sources(soc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FAMILY_REALTEK_BEE_IMAGE_HEADER bee_image_header.c)

zephyr_linker_sources(ROM_START SORT_KEY 0x0header bee_image_header.ld)
zephyr_linker_sources(SECTIONS bee_ext_ram.ld)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

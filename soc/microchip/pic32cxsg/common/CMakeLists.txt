# Makefile - Microchip PIC32CXSG MCU family
#
# Copyright (c) 2024 Microchip
# SPDX-License-Identifier: Apache-2.0

zephyr_sources(soc_port.c)

zephyr_sources_ifdef(CONFIG_BOOTLOADER_BOSSA bossa.c)

zephyr_sources_ifdef(CONFIG_SOC_SERIES_PIC32CXSG41 soc_pic32cxsg.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_PIC32CXSG60 soc_pic32cxsg.c)
zephyr_sources_ifdef(CONFIG_SOC_SERIES_PIC32CXSG61 soc_pic32cxsg.c)

zephyr_include_directories(.)

add_subdirectory(component)
add_subdirectory(instance)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

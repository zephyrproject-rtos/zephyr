# Copyright (c) 2021 Yonatan Schachter
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(soc.c)

zephyr_include_directories(.)

# CPU0 needs an IMAGE_DEF for the boot ROM, while CPU1 does not!
if(CONFIG_SOC_RP2350A_M33_CPU0)
set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld CACHE INTERNAL "")
else()
set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "SoC Linker script")
endif()

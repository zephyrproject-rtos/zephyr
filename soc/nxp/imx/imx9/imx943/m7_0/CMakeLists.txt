# Copyright 2025 NXP

zephyr_include_directories(.)

zephyr_library()
zephyr_library_sources(soc.c)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

# SPDX-FileCopyrightText: Copyright 2025-2026 NXP

zephyr_include_directories(.)

zephyr_library()
zephyr_library_sources(soc.c)
zephyr_library_sources_ifdef(CONFIG_SOC_EARLY_RESET_HOOK soc.S)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

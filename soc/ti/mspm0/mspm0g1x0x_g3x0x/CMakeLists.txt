# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MSPM0G350X)
    zephyr_compile_definitions(-D__MSPM0G3507__)
endif()
zephyr_sources(soc.c)
zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "")

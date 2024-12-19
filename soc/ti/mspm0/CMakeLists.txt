# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "SoC Linker Script")

add_subdirectory(${SOC_SERIES})

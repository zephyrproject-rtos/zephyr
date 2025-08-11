# SPDX-License-Identifier: Apache-2.0

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "SoC Linker Script")

add_subdirectory(common)

if(CONFIG_SOC_FAMILY_TI_MSPM0)
	string(TOUPPER ${CONFIG_SOC} SDK_SOC_SELECT)
	zephyr_compile_definitions(-D__${SDK_SOC_SELECT}__)
endif()

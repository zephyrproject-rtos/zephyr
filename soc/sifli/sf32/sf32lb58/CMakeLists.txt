zephyr_include_directories(.)

set(SOC_LINKER_SCRIPT ${ZEPHYR_BASE}/include/zephyr/arch/arm/cortex_m/scripts/linker.ld CACHE INTERNAL "SoC Linker script")

if(CONFIG_SOC_SF32LB586_HCPU)
	zephyr_compile_definitions(-DSOC_BF0_HCPU)
endif()

zephyr_include_directories(.)
zephyr_sources_ifdef(CONFIG_PM power.c )

set(SOC_LINKER_SCRIPT
    ${ZEPHYR_BASE}/include/zephyr/arch/dspic/linker.ld
    CACHE INTERNAL ""
)

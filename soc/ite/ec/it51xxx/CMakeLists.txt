zephyr_sources(soc.c vector.S)
zephyr_include_directories(.)

zephyr_sources_ifdef(CONFIG_SOC_IT51XXX_USE_ILM ../it8xxx2/ilm.c ilm_wrapper.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    CACHE INTERNAL "SoC Linker script ${SOC_NAME}"
)

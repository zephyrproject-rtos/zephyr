zephyr_sources(soc.c check_regs.c vector.S)
zephyr_include_directories(.)

zephyr_library_sources_ifndef(CONFIG_RISCV_ISA_EXT_M __arithmetic.S)
zephyr_sources_ifdef(CONFIG_SOC_IT8XXX2_USE_ILM ilm.c)

set(SOC_LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld
    CACHE INTERNAL "SoC Linker script ${SOC_NAME}"
)

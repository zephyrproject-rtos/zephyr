zephyr_include_directories(.)
zephyr_include_directories(${DFP_ROOT}/support/generic/h)
zephyr_sources_ifdef(CONFIG_PM power.c )

set(SOC_LINKER_SCRIPT
    ${ZEPHYR_BASE}/include/zephyr/arch/dspic/linker.ld
    CACHE INTERNAL ""
)

if(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK128MC106)
    zephyr_ld_options(-T ${ZEPHYR_BASE}/soc/microchip/dspic33/dspic33a/p33ak128mc106_regs.ld)
elseif(CONFIG_BOARD_DSPIC33A_CURIOSITY_P33AK512MPS512)
    zephyr_ld_options(-T ${ZEPHYR_BASE}/soc/microchip/dspic33/dspic33a/p33ak512mps512_regs.ld)
endif()
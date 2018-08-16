target_include_directories(drivers PRIVATE ${ZEPHYR_BASE}/subsys/net/l2)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_ETH_SAM_GMAC}      eth_sam_gmac.c
                                              phy_sam_gmac.c
            IFDEF:${CONFIG_ETH_STELLARIS}     eth_stellaris.c
            IFDEF:${CONFIG_ETH_DW}            eth_dw.c
            IFDEF:${CONFIG_ETH_E1000}         eth_e1000.c
            IFDEF:${CONFIG_ETH_ENC28J60}      eth_enc28j60.c
            IFDEF:${CONFIG_ETH_MCUX}          eth_mcux.c
            IFDEF:${CONFIG_ETH_STM32_HAL}     eth_stm32_hal.c
            IFDEF:${CONFIG_ETH_NATIVE_POSIX}  eth_native_posix.c
                                              eth_native_posix_adapt.c
)

if(CONFIG_ETH_NATIVE_POSIX)
    target_include_directories(drivers PRIVATE ${ZEPHYR_BASE}/subsys/net/l2)
    SET_SOURCE_FILES_PROPERTIES(${PRIVATE_SOURCES} PROPERTIES
                                COMPILE_DEFINITIONS NO_POSIX_CHEATS
                                COMPILE_DEFINITIONS _BSD_SOURCE
                                COMPILE_DEFINITIONS _DEFAULT_SOURCE
    )
endif()

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

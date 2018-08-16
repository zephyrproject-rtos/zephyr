
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_TEMP_NRF5} temp_nrf5.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

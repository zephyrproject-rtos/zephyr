zephyr_include_directories(.)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            wifi_winc1500_nm_bsp.c
            wifi_winc1500_nm_bus_wrapper.c
            wifi_winc1500.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

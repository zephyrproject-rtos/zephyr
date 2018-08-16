zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_HDC1008} hdc1008.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

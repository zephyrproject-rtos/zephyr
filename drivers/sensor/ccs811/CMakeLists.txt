
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_CCS811} ccs811.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

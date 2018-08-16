
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_ADXL362} adxl362.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

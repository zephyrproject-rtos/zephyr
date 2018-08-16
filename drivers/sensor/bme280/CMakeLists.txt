
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_BME280} bme280.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

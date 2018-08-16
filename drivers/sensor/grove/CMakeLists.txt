zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_GROVE_LIGHT_SENSOR}       light_sensor.c
            IFDEF:${CONFIG_GROVE_TEMPERATURE_SENSOR} temperature_sensor.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

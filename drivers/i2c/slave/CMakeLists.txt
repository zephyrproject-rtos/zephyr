zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_I2C_EEPROM_SLAVE} eeprom_slave.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

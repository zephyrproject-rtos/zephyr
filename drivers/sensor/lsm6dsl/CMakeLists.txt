
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LSM6DSL}            lsm6dsl.c
            IFDEF:${CONFIG_LSM6DSL_SPI}        lsm6dsl_spi.c
            IFDEF:${CONFIG_LSM6DSL_I2C}        lsm6dsl_i2c.c
            IFDEF:${CONFIG_LSM6DSL_TRIGGER}    lsm6dsl_trigger.c
            IFDEF:${CONFIG_LSM6DSL_SENSORHUB}  lsm6dsl_shub.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

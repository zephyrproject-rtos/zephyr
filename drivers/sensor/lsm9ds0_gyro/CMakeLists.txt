zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LSM9DS0_GYRO}              lsm9ds0_gyro.c
            IFDEF:${CONFIG_LSM9DS0_GYRO_TRIGGER_DRDY} lsm9ds0_gyro_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

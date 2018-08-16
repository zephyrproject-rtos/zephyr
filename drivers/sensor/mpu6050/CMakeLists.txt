zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_MPU6050}         mpu6050.c
            IFDEF:${CONFIG_MPU6050_TRIGGER} mpu6050_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            stm32cube_hal.c
            IFDEF:${CONFIG_ARM_MPU} arm_mpu_regions.c
)

target_sources(soc_arm PRIVATE ${PRIVATE_SOURCES})

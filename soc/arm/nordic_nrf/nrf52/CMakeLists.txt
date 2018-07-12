zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            power.c
            soc.c
            IFDEF:${CONFIG_ARM_MPU} mpu_regions.c
)

target_sources(soc_arm PRIVATE ${PRIVATE_SOURCES})

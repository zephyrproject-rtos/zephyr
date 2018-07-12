zephyr_include_directories(.)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            soc_pmc.c
            soc_gpio.c
            IFDEF:${CONFIG_ARM_MPU} arm_mpu_regions.c
)

target_sources(soc_arm PRIVATE ${PRIVATE_SOURCES})

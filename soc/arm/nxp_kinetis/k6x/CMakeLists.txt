zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            soc.c
            wdog.S
            IFDEF:${CONFIG_ARM_MPU} nxp_mpu_regions.c
)

target_sources(soc_arm PRIVATE ${PRIVATE_SOURCES})

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            pmodmux.c
            IFDEF:${CONFIG_ARC_MPU_ENABLE} arc_mpu_regions.c
)

target_sources(board PRIVATE ${PRIVATE_SOURCES})

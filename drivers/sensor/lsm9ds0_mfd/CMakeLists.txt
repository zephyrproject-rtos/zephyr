
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LSM9DS0_MFD} lsm9ds0_mfd.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

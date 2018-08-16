
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LSM6DS0} lsm6ds0.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

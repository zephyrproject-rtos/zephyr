
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            vl53l0x.c
            vl53l0x_platform.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})


zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_MAX44009} max44009.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

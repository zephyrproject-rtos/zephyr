
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_PMS7003} pms7003.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

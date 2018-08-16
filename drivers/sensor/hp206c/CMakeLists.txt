
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_HP206C} hp206c.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

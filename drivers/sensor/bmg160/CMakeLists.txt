
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_BMG160}         bmg160.c
            IFDEF:${CONFIG_BMG160_TRIGGER} bmg160_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

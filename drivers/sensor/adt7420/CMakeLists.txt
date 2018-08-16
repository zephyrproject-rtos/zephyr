
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_ADT7420}         adt7420.c
            IFDEF:${CONFIG_ADT7420_TRIGGER} adt7420_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

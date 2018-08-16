
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_SX9500}         sx9500.c
            IFDEF:${CONFIG_SX9500_TRIGGER} sx9500_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

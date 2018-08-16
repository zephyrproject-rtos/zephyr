
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_HTS221}         hts221.c
            IFDEF:${CONFIG_HTS221_TRIGGER} hts221_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

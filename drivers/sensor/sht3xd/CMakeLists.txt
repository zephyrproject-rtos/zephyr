
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_SHT3XD}         sht3xd.c
            IFDEF:${CONFIG_SHT3XD_TRIGGER} sht3xd_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

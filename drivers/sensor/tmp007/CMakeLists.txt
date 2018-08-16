
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_TMP007}         tmp007.c
            IFDEF:${CONFIG_TMP007_TRIGGER} tmp007_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})


zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_BMA280}         bma280.c
            IFDEF:${CONFIG_BMA280_TRIGGER} bma280_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

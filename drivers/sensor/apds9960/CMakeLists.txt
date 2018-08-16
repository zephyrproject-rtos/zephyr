zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_APDS9960} apds9960.c
            IFDEF:${CONFIG_APDS9960_TRIGGER} apds9960_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

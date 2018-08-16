
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_ISL29035}         isl29035.c
            IFDEF:${CONFIG_ISL29035_TRIGGER} isl29035_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

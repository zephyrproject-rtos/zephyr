
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LIS2DH}         lis2dh.c
            IFDEF:${CONFIG_LIS2DH_TRIGGER} lis2dh_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

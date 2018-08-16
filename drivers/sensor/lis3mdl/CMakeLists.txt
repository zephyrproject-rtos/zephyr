
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LIS3MDL}         lis3mdl.c
            IFDEF:${CONFIG_LIS3MDL_TRIGGER} lis3mdl_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

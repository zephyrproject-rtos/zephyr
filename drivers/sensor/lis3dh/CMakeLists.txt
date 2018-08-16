
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_LIS3DH}         lis3dh.c
            IFDEF:${CONFIG_LIS3DH_TRIGGER} lis3dh_trigger.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

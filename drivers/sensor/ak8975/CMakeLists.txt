
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_AK8975} ak8975.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

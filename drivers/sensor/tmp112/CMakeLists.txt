
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_TMP112} tmp112.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

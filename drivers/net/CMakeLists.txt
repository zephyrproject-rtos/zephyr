zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_SLIP}         slip.c
            IFDEF:${CONFIG_NET_LOOPBACK} loopback.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

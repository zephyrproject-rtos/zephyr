
zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            IFDEF:${CONFIG_DHT} dht.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

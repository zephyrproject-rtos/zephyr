zephyr_include_directories(./)

target_include_directories(drivers PRIVATE
  # IP headers
  ${ZEPHYR_BASE}/subsys/net/ip
)

zephyr_list(SOURCES
            OUTPUT PRIVATE_SOURCES
            eswifi_core.c
            eswifi_bus_spi.c
            eswifi_offload.c
)

target_sources(drivers PRIVATE ${PRIVATE_SOURCES})

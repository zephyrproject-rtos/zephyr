zephyr_library()

zephyr_library_sources(
  net_utils.c
  mqtt_pkt.c
  mqtt.c
  )

zephyr_link_interface_ifdef(CONFIG_MBEDTLS mbedTLS)
zephyr_library_link_libraries_ifdef(CONFIG_MBEDTLS mbedTLS)

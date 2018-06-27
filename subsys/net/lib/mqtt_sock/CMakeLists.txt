zephyr_library()

zephyr_library_sources(
  mqtt_decoder.c
  mqtt_encoder.c
  mqtt_rx.c
  mqtt_transport_socket_tcp.c
  mqtt_transport.c
  mqtt.c
  )

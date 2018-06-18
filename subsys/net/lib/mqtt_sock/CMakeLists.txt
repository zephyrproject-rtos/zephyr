zephyr_list(SOURCES
  OUTPUT PRIVATE_SOURCES
  mqtt_decoder.c
  mqtt_encoder.c
  mqtt_rx.c
  mqtt_transport_socket_tcp.c
  mqtt_transport.c
  mqtt.c
  IFDEF:${CONFIG_MQTT_LIB_TLS} mqtt_transport_socket_tls.c
)

target_sources(subsys_net PRIVATE ${PRIVATE_SOURCES})

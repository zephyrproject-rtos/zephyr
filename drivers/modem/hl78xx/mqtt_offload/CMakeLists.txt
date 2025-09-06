# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  mqtt_offload.c
  mqtt_offload_decoder.c
  mqtt_offload_encoder.c
  mqtt_offload_transport.c
  mqtt_offload_transport_mqtt.c
)

zephyr_library_sources_ifdef(CONFIG_MQTT_OFFLOAD_LIB_TLS
  mqtt_offload_transport_tls_mqtt.c
)

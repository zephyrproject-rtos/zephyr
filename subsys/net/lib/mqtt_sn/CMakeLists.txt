# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  mqtt_sn.c
  mqtt_sn_decoder.c
  mqtt_sn_encoder.c
)

zephyr_library_sources_ifdef(CONFIG_MQTT_SN_TRANSPORT_UDP
  mqtt_sn_transport_udp.c
)

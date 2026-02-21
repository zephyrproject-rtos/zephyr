# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  tftp_client.c
  )

if(CONFIG_TFTPS)
zephyr_library_sources(
  tftp_server.c
  tftp_server_udp.c
  )
endif()

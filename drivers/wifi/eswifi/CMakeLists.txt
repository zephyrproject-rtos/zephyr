# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_ESWIFI)

  zephyr_library_include_directories(
  # IP headers
  ${ZEPHYR_BASE}/subsys/net/ip
  )

  zephyr_library_sources(
    eswifi_core.c
    eswifi_offload.c
    eswifi_socket.c
    )

  zephyr_library_sources_ifdef(CONFIG_WIFI_ESWIFI_BUS_SPI eswifi_bus_spi.c)
  zephyr_library_sources_ifdef(CONFIG_WIFI_ESWIFI_BUS_UART eswifi_bus_uart.c)

  zephyr_library_sources_ifdef(CONFIG_WIFI_ESWIFI_SHELL eswifi_shell.c)

  zephyr_library_sources_ifdef(CONFIG_NET_SOCKETS_OFFLOAD eswifi_socket_offload.c)
endif()

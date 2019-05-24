# SPDX-License-Identifier: Apache-2.0

if(CONFIG_WIFI_ESWIFI)
  zephyr_include_directories(./)

  zephyr_library_include_directories(
  # IP headers
  ${ZEPHYR_BASE}/subsys/net/ip
  )

  zephyr_sources(
    eswifi_core.c
    eswifi_bus_spi.c
    eswifi_offload.c
    )
endif()

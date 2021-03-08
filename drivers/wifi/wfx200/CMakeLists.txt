# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/l2)

if(CONFIG_WIFI_WFX200)
  zephyr_include_directories(./)

  zephyr_library_sources(
    wfx200.c
    wfx200_host.c
    wfx200_events.c
  )
endif()

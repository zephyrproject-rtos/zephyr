# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LORAWAN_SERVICES)

  zephyr_library_sources(lorawan_services.c)

  zephyr_library_sources_ifdef(
    CONFIG_LORAWAN_APP_CLOCK_SYNC
    clock_sync.c
  )

  zephyr_library_sources_ifdef(
    CONFIG_LORAWAN_REMOTE_MULTICAST
    multicast.c
  )

endif()

# SPDX-License-Identifier: Apache-2.0

if(CONFIG_LORAWAN_SERVICES)

  zephyr_library()

  zephyr_library_sources(lorawan_services.c)

  zephyr_library_sources_ifdef(
    CONFIG_LORAWAN_APP_CLOCK_SYNC
    clock_sync.c
  )

  zephyr_library_sources_ifdef(
    CONFIG_LORAWAN_FRAG_TRANSPORT
    frag_flash.c
    frag_transport.c
  )

  zephyr_library_sources_ifdef(
    CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_LOWMEM
    frag_decoder_lowmem.c
  )

  if(CONFIG_LORAWAN_FRAG_TRANSPORT_DECODER_SEMTECH)
    zephyr_library_include_directories(
      ${ZEPHYR_LORAMAC_NODE_MODULE_DIR}/src/apps/LoRaMac/common/LmHandler/packages
    )
  endif()

endif()

# The multicast service is loramac-node specific
set(ZEPHYR_CURRENT_LIBRARY loramac-node)
zephyr_library_sources_ifdef(CONFIG_LORAWAN_REMOTE_MULTICAST multicast.c)

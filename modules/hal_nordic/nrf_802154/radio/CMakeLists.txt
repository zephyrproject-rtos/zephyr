# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

set(NRF_802154_DRIVER_ROOT ${ZEPHYR_CURRENT_MODULE_DIR}/drivers/nrf_radio_802154)

include(${NRF_802154_DRIVER_ROOT}/nrf_802154_driver_sources.cmake)

zephyr_library()

zephyr_include_directories(${NRF_802154_DRIVER_INCLUDE_DIRS})

if (CONFIG_SOC_SERIES_NRF52X)
zephyr_library_sources(
  ${NRF_802154_DRIVER_SOURCES_NRF52}
  )
elseif (CONFIG_SOC_SERIES_NRF53X)
zephyr_library_sources(
  ${NRF_802154_DRIVER_SOURCES_NRF53}
  )
else()
    message(FATAL_ERROR "SoC unsupported by this module")
endif()

if (CONFIG_NRF_802154_SL_OPENSOURCE)
zephyr_library_sources(
  ${NRF_802154_DRIVER_SOURCES_DIRECT}
  )

else()
zephyr_library_sources(
  ${NRF_802154_DRIVER_SOURCES_SWI}
  )

endif()

zephyr_library_sources(${CMAKE_CURRENT_SOURCE_DIR}/platform/nrf_802154_random_zephyr.c)

if(     CONFIG_NRF_802154_CCA_MODE_ED)
  set(radio_cca_mode NRF_RADIO_CCA_MODE_ED)

elseif( CONFIG_NRF_802154_CCA_MODE_CARRIER)
  set(radio_cca_mode NRF_RADIO_CCA_MODE_CARRIER)

elseif( CONFIG_NRF_802154_CCA_MODE_CARRIER_AND_ED)
  set(radio_cca_mode NRF_RADIO_CCA_MODE_CARRIER_AND_ED)

elseif( CONFIG_NRF_802154_CCA_MODE_CARRIER_OR_ED)
  set(radio_cca_mode NRF_RADIO_CCA_MODE_CARRIER_OR_ED)

endif()

zephyr_compile_definitions(
  # Radio driver shim layer uses raw api
  NRF_802154_USE_RAW_API=1

  # Number of slots containing short addresses of nodes for which
  # pending data is stored.
  NRF_802154_PENDING_SHORT_ADDRESSES=${CONFIG_NRF_802154_PENDING_SHORT_ADDRESSES}

  # Number of slots containing extended addresses of nodes for which
  # pending data is stored.
  NRF_802154_PENDING_EXTENDED_ADDRESSES=${CONFIG_NRF_802154_PENDING_EXTENDED_ADDRESSES}

  # Number of buffers in receive queue.
  NRF_802154_RX_BUFFERS=${CONFIG_NRF_802154_RX_BUFFERS}

  # CCA mode
  NRF_802154_CCA_MODE_DEFAULT=${radio_cca_mode}

  # CCA mode options
  NRF_802154_CCA_CORR_LIMIT_DEFAULT=${CONFIG_NRF_802154_CCA_CORR_LIMIT}
  NRF_802154_CCA_CORR_THRESHOLD_DEFAULT=${CONFIG_NRF_802154_CCA_CORR_THRESHOLD}
  NRF_802154_CCA_ED_THRESHOLD_DEFAULT=${CONFIG_NRF_802154_CCA_ED_THRESHOLD}

  # Enable CSMA/CA
  NRF_802154_CSMA_CA_ENABLED=1
  NRF_802154_TX_STARTED_NOTIFY_ENABLED=1

  # ACK timeout
  NRF_802154_ACK_TIMEOUT_ENABLED=1
)

if (NOT CONFIG_IEEE802154_NRF5 AND NOT CONFIG_IEEE802154_NRF5_EXT_IRQ_MGMT)
  zephyr_compile_definitions(NRF_802154_INTERNAL_RADIO_IRQ_HANDLING=1)
else ()
  zephyr_compile_definitions(NRF_802154_INTERNAL_RADIO_IRQ_HANDLING=0)
endif ()

if (CONFIG_NRF_802154_SL_OPENSOURCE OR CONFIG_SOC_SERIES_NRF53X)
  zephyr_compile_definitions(
    # Disable Frame Timestamps
    NRF_802154_FRAME_TIMESTAMP_ENABLED=0
    # Disable DTRX
    NRF_802154_DELAYED_TRX_ENABLED=0
    # Disable IFS
    NRF_802154_IFS_ENABLED=0
  )

else()
  zephyr_compile_definitions(
    # Enable Frame Timestamps
    NRF_802154_FRAME_TIMESTAMP_ENABLED=1
    # Enable DTRX
    NRF_802154_DELAYED_TRX_ENABLED=1
    # Enable IFS
    NRF_802154_IFS_ENABLED=1
  )
endif()

if (NOT CONFIG_NRF_802154_SL_OPENSOURCE)
  zephyr_compile_definitions(
    NRF_802154_VERIFY_PERIPHS_ALLOC_AGAINST_MPSL=1
  )
endif()

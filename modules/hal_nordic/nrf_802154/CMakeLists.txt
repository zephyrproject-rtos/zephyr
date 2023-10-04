# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_library_named(nrf-802154-platform)
zephyr_interface_library_named(zephyr-802154-interface)

if (CONFIG_NRF_802154_RADIO_DRIVER)
  target_sources(nrf-802154-platform
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/radio/platform/nrf_802154_random_zephyr.c
      ${CMAKE_CURRENT_SOURCE_DIR}/sl_opensource/platform/nrf_802154_gpiote_crit_sect.c
      ${CMAKE_CURRENT_SOURCE_DIR}/sl_opensource/platform/nrf_802154_clock_zephyr.c
      ${CMAKE_CURRENT_SOURCE_DIR}/sl_opensource/platform/nrf_802154_gpiote_zephyr.c
      ${CMAKE_CURRENT_SOURCE_DIR}/sl_opensource/platform/nrf_802154_irq_zephyr.c
      ${CMAKE_CURRENT_SOURCE_DIR}/sl_opensource/platform/nrf_802154_temperature_zephyr.c
  )

  target_compile_definitions(zephyr-802154-interface
    INTERFACE
      # CCA mode options
      NRF_802154_CCA_CORR_LIMIT_DEFAULT=${CONFIG_NRF_802154_CCA_CORR_LIMIT}
      NRF_802154_CCA_CORR_THRESHOLD_DEFAULT=${CONFIG_NRF_802154_CCA_CORR_THRESHOLD}
      NRF_802154_CCA_ED_THRESHOLD_DEFAULT=${CONFIG_NRF_802154_CCA_ED_THRESHOLD}
  )

  if (CONFIG_NRF_802154_CCA_MODE_ED)
    target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_CCA_MODE_DEFAULT=NRF_RADIO_CCA_MODE_ED)
  elseif (CONFIG_NRF_802154_CCA_MODE_CARRIER)
    target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_CCA_MODE_DEFAULT=NRF_RADIO_CCA_MODE_CARRIER)
  elseif (CONFIG_NRF_802154_CCA_MODE_CARRIER_AND_ED)
    target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_CCA_MODE_DEFAULT=NRF_RADIO_CCA_MODE_CARRIER_AND_ED)
  elseif (CONFIG_NRF_802154_CCA_MODE_CARRIER_OR_ED)
    target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_CCA_MODE_DEFAULT=NRF_RADIO_CCA_MODE_CARRIER_OR_ED)
  endif()
endif ()

if (CONFIG_NRF_802154_SERIALIZATION)
  target_sources(nrf-802154-platform
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/serialization/platform/nrf_802154_serialization_crit_sect.c
      ${CMAKE_CURRENT_SOURCE_DIR}/serialization/platform/nrf_802154_spinel_log.c
      ${CMAKE_CURRENT_SOURCE_DIR}/serialization/platform/nrf_802154_spinel_backend_ipc.c
      ${CMAKE_CURRENT_SOURCE_DIR}/serialization/platform/nrf_802154_spinel_response_notifier.c
  )
endif ()

if (CONFIG_NRF_802154_SER_RADIO)
    target_sources(nrf-802154-platform
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/serialization/platform/nrf_802154_init_net.c
    )
endif ()

target_compile_definitions(zephyr-802154-interface
  INTERFACE
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

    NRF_802154_TX_STARTED_NOTIFY_ENABLED=1

    # ACK timeout
    NRF_802154_ACK_TIMEOUT_ENABLED=1
)

if (CONFIG_NRF_802154_ENCRYPTION)
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_ENCRYPTION_ENABLED=1)
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_SECURITY_WRITER_ENABLED=1)
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_IE_WRITER_ENABLED=1)
else ()
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_ENCRYPTION_ENABLED=0)
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_SECURITY_WRITER_ENABLED=0)
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_IE_WRITER_ENABLED=0)
endif()

if (NOT CONFIG_IEEE802154_NRF5 AND NOT CONFIG_IEEE802154_NRF5_EXT_IRQ_MGMT AND CONFIG_NRF_802154_SL_OPENSOURCE)
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_INTERNAL_RADIO_IRQ_HANDLING=1)
else()
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_INTERNAL_RADIO_IRQ_HANDLING=0)
endif()

if (CONFIG_NRF_802154_CARRIER_FUNCTIONS)
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_CARRIER_FUNCTIONS_ENABLED=1)
else()
  target_compile_definitions(zephyr-802154-interface INTERFACE NRF_802154_CARRIER_FUNCTIONS_ENABLED=0)
endif()

set(NRF52_SERIES  ${CONFIG_SOC_SERIES_NRF52X})
set(NRF53_SERIES  ${CONFIG_SOC_SERIES_NRF53X})
set(SER_HOST      ${CONFIG_NRF_802154_SER_HOST})
set(SL_OPENSOURCE ${CONFIG_NRF_802154_SL_OPENSOURCE})

if(CONFIG_NRF_802154_SOURCE_HAL_NORDIC)
  add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers/nrf_802154 nrf_802154)

  target_link_libraries(nrf-802154-driver-interface INTERFACE zephyr-802154-interface)
  target_link_libraries(nrf-802154-serialization-interface INTERFACE zephyr-802154-interface)
endif()


if (CONFIG_NRF_802154_RADIO_DRIVER)
  zephyr_append_cmake_library(nrf-802154-driver)
  zephyr_append_cmake_library(nrf-802154-sl)
  target_link_libraries(zephyr_interface INTERFACE nrf-802154-driver-interface)
  # This link is needed to resolve circular dependency between SL and platform
  target_link_libraries(nrf-802154-platform PRIVATE nrf-802154-sl)
endif ()

if (CONFIG_NRF_802154_SERIALIZATION)
  zephyr_append_cmake_library(nrf-802154-serialization)
  target_link_libraries(zephyr_interface INTERFACE nrf-802154-serialization-interface)
endif ()

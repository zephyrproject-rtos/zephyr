# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if (CONFIG_NRF_802154_RADIO_DRIVER OR CONFIG_NRF_802154_SERIALIZATION)
  add_subdirectory(nrf_802154)
endif (CONFIG_NRF_802154_RADIO_DRIVER OR CONFIG_NRF_802154_SERIALIZATION)

add_subdirectory_ifdef(CONFIG_HAS_NRFX nrfx)
add_subdirectory_ifdef(CONFIG_HAS_NRFS nrfs)

if(CONFIG_NRF_REGTOOL_GENERATE_UICR)
  list(APPEND nrf_regtool_components GENERATE:UICR)
endif()
if(DEFINED nrf_regtool_components)
  find_package(nrf-regtool 5.6.0 REQUIRED
    COMPONENTS ${nrf_regtool_components}
    PATHS ${CMAKE_CURRENT_LIST_DIR}/nrf-regtool
    NO_CMAKE_PATH
    NO_CMAKE_ENVIRONMENT_PATH
    NO_SYSTEM_ENVIRONMENT_PATH
    NO_CMAKE_PACKAGE_REGISTRY
    NO_CMAKE_SYSTEM_PATH
    NO_CMAKE_SYSTEM_PACKAGE_REGISTRY
  )
endif()

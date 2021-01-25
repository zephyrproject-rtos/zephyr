# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_NRF_802154_RADIO_DRIVER OR CONFIG_NRF_802154_SERIALIZATION)
  add_subdirectory(nrf_802154)
endif ()

add_subdirectory_ifdef(CONFIG_HAS_NRFX   nrfx)

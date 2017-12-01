if(CONFIG_IEEE802154_NRF5 OR CONFIG_IEEE802154_NRF5_RAW)
  zephyr_sources(nrf_drv_radio802154.c)
endif()

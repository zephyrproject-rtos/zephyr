if(CONFIG_IEEE802154_NRF5 OR CONFIG_IEEE802154_NRF5_RAW)
  zephyr_sources(nrf_drv_radio802154.c)

  if(     CONFIG_IEEE802154_NRF5_CCA_MODE_ED)
    set(radio_cca_mode NRF_RADIO_CCA_MODE_ED)

  elseif( CONFIG_IEEE802154_NRF5_CCA_MODE_CARRIER)
    set(radio_cca_mode NRF_RADIO_CCA_MODE_CARRIER)

  elseif( CONFIG_IEEE802154_NRF5_CCA_MODE_CARRIER_AND_ED)
    set(radio_cca_mode NRF_RADIO_CCA_MODE_CARRIER_AND_ED)

  elseif( CONFIG_IEEE802154_NRF5_CCA_MODE_CARRIER_OR_ED)
    set(radio_cca_mode NRF_RADIO_CCA_MODE_CARRIER_OR_ED)

  endif()

  zephyr_compile_definitions(
    # Number of slots containing short addresses of nodes for which
    # pending data is stored.
    RADIO_PENDING_SHORT_ADDRESSES=1

    # Number of slots containing extended addresses of nodes for which
    # pending data is stored.
    RADIO_PENDING_EXTENDED_ADDRESSES=1

    # Number of buffers in receive queue.
    RADIO_RX_BUFFERS=1

    # CCA mode
    RADIO_CCA_MODE=${radio_cca_mode}

    # CCA mode options
    RADIO_CCA_CORR_LIMIT=${CONFIG_IEEE802154_NRF5_CCA_CORR_LIMIT}
    RADIO_CCA_CORR_THRESHOLD=${CONFIG_IEEE802154_NRF5_CCA_CORR_THRESHOLD}
    RADIO_CCA_ED_THRESHOLD=${CONFIG_IEEE802154_NRF5_CCA_ED_THRESHOLD}
    )
endif()

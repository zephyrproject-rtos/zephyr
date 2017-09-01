if(CONFIG_IEEE802154_MCR20A)
  zephyr_include_directories(mcr20a)
endif()

if(CONFIG_ETH_MCUX)
  zephyr_include_directories(phyksz8081)
endif()

zephyr_sources_ifdef(CONFIG_ETH_MCUX phyksz8081/fsl_phy.c)

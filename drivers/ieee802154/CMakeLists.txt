# SPDX-License-Identifier: Apache-2.0

zephyr_sources_ifdef(CONFIG_IEEE802154_CC2520 ieee802154_cc2520.c)
zephyr_sources_ifdef(CONFIG_IEEE802154_KW41Z ieee802154_kw41z.c)
zephyr_sources_ifdef(CONFIG_IEEE802154_UPIPE ieee802154_uart_pipe.c)
zephyr_sources_ifdef(CONFIG_IEEE802154_MCR20A ieee802154_mcr20a.c)
zephyr_sources_ifdef(CONFIG_IEEE802154_NRF5 ieee802154_nrf5.c)
zephyr_sources_ifdef(CONFIG_IEEE802154_CC1200 ieee802154_cc1200.c)
zephyr_sources_ifdef(CONFIG_IEEE802154_CC13XX_CC26XX ieee802154_cc13xx_cc26xx.c)

if(CONFIG_NET_L2_OPENTHREAD)
  # This driver calls DEVICE_INIT with the context of openthread. The
  # context of openthread is defined in one of OpenThread's header
  # files so we need express that this driver depends on OpenThread
  # being downloaded to make sure that we don't build this driver
  # before all of it's header file dependencies are met.
  add_dependencies(${ZEPHYR_CURRENT_LIBRARY} ot)
endif()

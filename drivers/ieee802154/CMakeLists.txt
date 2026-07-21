# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BUILD_ONLY_NO_BLOBS)
  message(WARNING "
  ---------------------------------------------------------------------------
  Building only the IEEE802154 driver without binary blobs and patches.
  This is only for building (CI) purposes and will not work on a real device.
  ---------------------------------------------------------------------------
  ")
else()

if(DT_HAS_ST_STM32WBA_IEEE802154_ENABLED)
  zephyr_blobs_verify(MODULE hal_stm32 REQUIRED)
endif()

endif() # CONFIG_BUILD_ONLY_NO_BLOBS

zephyr_library()

# zephyr-keep-sorted-start
zephyr_library_sources_ifdef(CONFIG_IEEE802154_CC1200 ieee802154_cc1200.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_CC13XX_CC26XX ieee802154_cc13xx_cc26xx.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_CC13XX_CC26XX_SUB_GHZ ieee802154_cc13xx_cc26xx_subg.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_CC2520 ieee802154_cc2520.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_DW1000 ieee802154_dw1000.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_ESP32 ieee802154_esp32.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_KW41Z ieee802154_kw41z.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_MCR20A ieee802154_mcr20a.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_MCXW ieee802154_mcxw.c ieee802154_mcxw_utils.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_NRF5 ieee802154_nrf5.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_RF2XX ieee802154_rf2xx.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_RF2XX ieee802154_rf2xx_iface.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_SILABS_EFR32 ieee802154_silabs_efr32.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_STM32WBA ieee802154_stm32wba.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_TELINK_B91 ieee802154_b91.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_UPIPE ieee802154_uart_pipe.c)
# zephyr-keep-sorted-stop

# SPDX-License-Identifier: Apache-2.0

zephyr_library()

# Silabs EFR32: link proprietary OpenThread PAL lib (provides sl_openthread_init to unlock RAIL for OpenThread).
if(CONFIG_IEEE802154_SILABS_EFR32)
  if(DEFINED ZEPHYR_HAL_SILABS_MODULE_DIR)
    set(_hal_silabs_dir "${ZEPHYR_HAL_SILABS_MODULE_DIR}")
  elseif(DEFINED ZEPHYR_BASE)
    get_filename_component(_hal_silabs_dir "${ZEPHYR_BASE}/../modules/hal/silabs" ABSOLUTE)
  else()
    get_filename_component(_hal_silabs_dir "${CMAKE_CURRENT_LIST_DIR}/../../../../modules/hal/silabs" ABSOLUTE)
  endif()
  set(_sl_openthread_lib "${_hal_silabs_dir}/zephyr/blobs/simplicity_sdk/protocol/openthread/platform-abstraction/efr32/libsl_openthread_cm33_gcc.a")
  if(EXISTS "${_sl_openthread_lib}")
    zephyr_library_link_libraries("${_sl_openthread_lib}")
  else()
    message(FATAL_ERROR "IEEE802154_SILABS_EFR32: Silabs OpenThread lib not found at ${_sl_openthread_lib}")
  endif()
endif()

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

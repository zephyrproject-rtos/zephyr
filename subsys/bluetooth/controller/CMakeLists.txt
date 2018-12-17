zephyr_library()
zephyr_library_sources(
  util/mem.c
  util/memq.c
  util/mayfly.c
  util/util.c
  ticker/ticker.c
  ll_sw/ll_addr.c
  ll_sw/ll_tx_pwr.c
  hci/hci_driver.c
  hci/hci.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_CRYPTO
  crypto/crypto.c
  )

if(CONFIG_BT_LL_SW)
  zephyr_library_sources(
    ll_sw/ctrl.c
    ll_sw/ll.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_BROADCASTER
    ll_sw/ll_adv.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_OBSERVER
    ll_sw/ll_scan.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_FILTER
    ll_sw/ll_filter.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CENTRAL
    ll_sw/ll_master.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_EXT
    ll_sw/ll_adv_aux.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_DTM
    ll_sw/ll_test.c
    )
endif()

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_FILTER
  ll_sw/ll_filter.c
  )

zephyr_library_sources_ifdef(
  CONFIG_SOC_COMPATIBLE_NRF
  ll_sw/nordic/hal/nrf5/cntr.c
  ll_sw/nordic/hal/nrf5/ecb.c
  ll_sw/nordic/hal/nrf5/radio/radio.c
  ll_sw/nordic/hal/nrf5/mayfly.c
  ll_sw/nordic/hal/nrf5/ticker.c
  )

zephyr_library_include_directories(
  .
  include
  ll_sw
  )

if(CONFIG_SOC_COMPATIBLE_NRF)
  zephyr_library_include_directories(
    ll_sw/nordic
    )
endif()

zephyr_library_compile_options_ifdef(
  CONFIG_BT_CTLR_FAST_ENC
  ${OPTIMIZE_FOR_SPEED_FLAG}
  )

zephyr_library_link_libraries(subsys__bluetooth)

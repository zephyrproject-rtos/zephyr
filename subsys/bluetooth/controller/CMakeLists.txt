zephyr_library()
zephyr_library_sources(
  util/mem.c
  util/memq.c
  util/mayfly.c
  util/util.c
  ticker/ticker.c
  ll_sw/ll_addr.c
  ll_sw/ll_tx_pwr.c
  crypto/crypto.c
  hci/hci_driver.c
  hci/hci.c
  )

if(CONFIG_BT_LL_SW)
  zephyr_library_sources(
    ll_sw/ctrl.c
    ll_sw/ll.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CENTRAL
    ll_sw/ll_master.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_HCI_MESH_EXT
    ll_sw/ll_mesh.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_DTM
    ll_sw/ll_test.c
    )
endif()

if(CONFIG_BT_LL_SW_SPLIT)
  zephyr_library_sources(
    ll_sw/ull.c
    ll_sw/lll.c
    ll_sw/lll_chan.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_TMP
    ll_sw/ull_tmp.c
    ll_sw/lll_tmp.c
    )
endif()

if(CONFIG_BT_BROADCASTER)
  zephyr_library_sources(
    ll_sw/ull_adv.c
    ll_sw/lll_adv.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_EXT
    ll_sw/ull_adv_aux.c
    )
endif()

zephyr_library_sources_ifdef(
  CONFIG_BT_OBSERVER
  ll_sw/ll_scan.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_FILTER
  ll_sw/ll_filter.c
  )

zephyr_library_sources_ifdef(
  CONFIG_SOC_FAMILY_NRF
  hal/nrf5/cntr.c
  hal/nrf5/ecb.c
  hal/nrf5/radio/radio.c
  hal/nrf5/mayfly.c
  hal/nrf5/ticker.c
  )

zephyr_library_include_directories(
  .
  include
  )

zephyr_library_compile_options_ifdef(
  CONFIG_BT_CTLR_FAST_ENC
  -Ofast
  )

zephyr_library_link_libraries(subsys__bluetooth)

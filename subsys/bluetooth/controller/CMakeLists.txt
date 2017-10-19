zephyr_library()
zephyr_library_sources(
  util/mem.c
  util/memq.c
  util/mayfly.c
  util/util.c
  hal/nrf5/cntr.c
  hal/nrf5/rand.c
  hal/nrf5/ecb.c
  hal/nrf5/radio.c
  ticker/ticker.c
  ll_sw/ctrl.c
  ll_sw/crypto.c
  ll_sw/ll.c
  ll_sw/ll_filter.c
  hci/hci_driver.c
  hci/hci.c
  )

zephyr_library_sources_ifdef(CONFIG_BT_BROADCASTER ll_sw/ll_adv.c)
zephyr_library_sources_ifdef(CONFIG_BT_OBSERVER    ll_sw/ll_scan.c)
zephyr_library_sources_ifdef(CONFIG_BT_CENTRAL     ll_sw/ll_master.c)
zephyr_library_sources_ifdef(CONFIG_BT_CTLR_DTM    ll_sw/ll_test.c)

zephyr_library_include_directories(
  .
  util
  hal
  ticker
  ll
  include
)

zephyr_library_compile_options_ifdef(
  CONFIG_BT_CTLR_FAST_ENC
  -Ofast
  )

zephyr_library_link_libraries(subsys__bluetooth)

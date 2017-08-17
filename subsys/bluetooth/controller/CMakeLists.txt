# TODO: KBuild was structured differently here. Should we
# conditionally include ll_adv, ll_scan, ll_master, etc., like KBuild
# did?

add_library(subsys__bluetooth__controller STATIC
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

target_sources_ifdef(CONFIG_BT_BROADCASTER subsys__bluetooth__controller PRIVATE ll_sw/ll_adv.c)
target_sources_ifdef(CONFIG_BT_OBSERVER    subsys__bluetooth__controller PRIVATE ll_sw/ll_scan.c)
target_sources_ifdef(CONFIG_BT_CENTRAL     subsys__bluetooth__controller PRIVATE ll_sw/ll_master.c)

zephyr_library(subsys__bluetooth__controller)

target_include_directories(subsys__bluetooth__controller PRIVATE
  .
  util
  hal
  ticker
  ll
  include
)

target_link_libraries(subsys__bluetooth__controller subsys__bluetooth)

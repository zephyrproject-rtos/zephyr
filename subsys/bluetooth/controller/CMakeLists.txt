# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BT_CTLR_ADVANCED_FEATURES)
  message(WARNING "\nCONFIG_BT_CTLR_ADVANCED_FEATURES=y, Advanced Features' "
                  "default value change could change Zephyr Bluetooth "
                  "Controller's functional behavior.")
endif()

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources(
  util/mem.c
  util/memq.c
  util/mayfly.c
  util/dbuf.c
  util/util.c
  ticker/ticker.c
  ll_sw/ll_feat.c
  ll_sw/ll_tx_pwr.c
  ll_sw/ll_addr.c
  ll_sw/ull.c
  ll_sw/lll_common.c
  hci/hci_driver.c
  hci/hci.c
  )

if(CONFIG_BT_BROADCASTER)
  zephyr_library_sources(
    ll_sw/ull_adv.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_EXT
    ll_sw/ull_adv_aux.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_PERIODIC
    ll_sw/ull_adv_sync.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_ISO
    ll_sw/ull_adv_iso.c
    )
endif()

if(CONFIG_BT_OBSERVER)
  zephyr_library_sources(
    ll_sw/ull_scan.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_EXT
    ll_sw/ull_scan_aux.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_SYNC_PERIODIC
    ll_sw/ull_sync.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_SYNC_ISO
    ll_sw/ull_sync_iso.c
    )
endif()

if(CONFIG_BT_CONN)
  zephyr_library_sources(
    ll_sw/ull_conn.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_LE_ENC
    ll_sw/ull_llcp_enc.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_PHY
    ll_sw/ull_llcp_phy.c
    )
  if (CONFIG_BT_CTLR_PERIPHERAL_ISO OR
      CONFIG_BT_CTLR_CENTRAL_ISO)
    zephyr_library_sources(
      ll_sw/ull_llcp_cc.c
      )
  endif()
  zephyr_library_sources(
    ll_sw/ull_tx_queue.c
    ll_sw/ull_llcp.c
    ll_sw/ull_llcp_common.c
    ll_sw/ull_llcp_local.c
    ll_sw/ull_llcp_pdu.c
    ll_sw/ull_llcp_conn_upd.c
    ll_sw/ull_llcp_chmu.c
    ll_sw/ull_llcp_remote.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_PERIPHERAL
    ll_sw/ull_peripheral.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_PERIPHERAL_ISO
    ll_sw/ull_peripheral_iso.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CENTRAL
    ll_sw/ull_central.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_CENTRAL_ISO
    ll_sw/ull_central_iso.c
    )
endif()

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_SCHED_ADVANCED
  ll_sw/ull_sched.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_DF
  ll_sw/ull_df.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_CONN_ISO
  ll_sw/ull_conn_iso.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_ISO
  ll_sw/ull_iso.c
  ll_sw/isoal.c
  )

if(CONFIG_BT_CONN OR
   (CONFIG_BT_BROADCASTER AND
    CONFIG_BT_CTLR_ADV_EXT) OR
   CONFIG_BT_CTLR_ADV_PERIODIC OR
   CONFIG_BT_CTLR_SYNC_PERIODIC)
  zephyr_library_sources(
    ll_sw/ull_chan.c
    ll_sw/lll_chan.c
    )
endif()

if(CONFIG_BT_CTLR_FILTER_ACCEPT_LIST OR
   CONFIG_BT_CTLR_SYNC_PERIODIC_ADV_LIST)
  zephyr_library_sources(
    ll_sw/ull_filter.c
    )
endif()

zephyr_library_sources_ifdef(
  CONFIG_BT_HCI_MESH_EXT
  ll_sw/ll_mesh.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_SETTINGS
  ll_sw/ll_settings.c
  )

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_CRYPTO
  crypto/crypto.c
  )

zephyr_library_sources_ifdef(
  CONFIG_SOC_FLASH_NRF_RADIO_SYNC_TICKER
  flash/soc_flash_nrf_ticker.c
  )

zephyr_library_include_directories(
  .
  include
  )

zephyr_library_include_directories_ifdef(
  CONFIG_BT_CTLR_CRYPTO
  ../crypto
  )

zephyr_library_include_directories_ifdef(
  CONFIG_SOC_FLASH_NRF_RADIO_SYNC_TICKER
  ${ZEPHYR_BASE}/drivers/flash
  )

zephyr_library_include_directories_ifdef(
  CONFIG_SOC_COMPATIBLE_NRF
  ll_sw/nordic
  ll_sw/nordic/hci
  )

zephyr_library_include_directories_ifdef(
  CONFIG_SOC_OPENISA_RV32M1
  ll_sw/openisa
  ll_sw/openisa/hci
  )

add_subdirectory_ifdef(
  CONFIG_BT_CTLR_COEX_DRIVERS
  coex
  )

add_subdirectory_ifdef(
  CONFIG_SOC_COMPATIBLE_NRF
  ll_sw/nordic
  )

add_subdirectory_ifdef(
  CONFIG_SOC_OPENISA_RV32M1
  ll_sw/openisa
  )

zephyr_library_compile_options_ifdef(
  CONFIG_BT_CTLR_OPTIMIZE_FOR_SPEED
  ${OPTIMIZE_FOR_SPEED_FLAG}
  )

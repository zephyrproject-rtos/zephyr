# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources(
  lll/lll.c
  lll/lll_clock.c
  )
if (CONFIG_BT_BROADCASTER)
  zephyr_library_sources(
    lll/lll_adv.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_EXT
    lll/lll_adv_aux.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_PERIODIC
    lll/lll_adv_sync.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_ISO
    lll/lll_adv_iso.c
    )
endif()
if(CONFIG_BT_OBSERVER)
  zephyr_library_sources(
    lll/lll_scan.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_ADV_EXT
    lll/lll_scan_aux.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_SYNC_PERIODIC
    lll/lll_sync.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_SYNC_ISO
    lll/lll_sync_iso.c
    )
endif()
if(CONFIG_BT_CONN)
  zephyr_library_sources(
    lll/lll_conn.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_PERIPHERAL
    lll/lll_peripheral.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CENTRAL
    lll/lll_central.c
  )
endif()
if(CONFIG_BT_CTLR_CONN_ISO)
  zephyr_library_sources(
    lll/lll_conn_iso.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_CENTRAL_ISO
    lll/lll_central_iso.c
    )
  zephyr_library_sources_ifdef(
    CONFIG_BT_CTLR_PERIPHERAL_ISO
    lll/lll_peripheral_iso.c
    )
endif()
zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_DF
  lll/lll_df.c
)
zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_DTM
  lll/lll_test.c
)
zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_PROFILE_ISR
  lll/lll_prof.c
)

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_ISO_VENDOR_DATA_PATH
  ull/ull_iso_vendor.c
  )

zephyr_library_sources(
  hci/hci_vendor.c
  )

zephyr_library_sources(
  hal/nrf5/cntr.c
  hal/nrf5/ecb.c
  hal/nrf5/radio/radio.c
  hal/nrf5/mayfly.c
  hal/nrf5/ticker.c
  )
if(CONFIG_BT_CTLR_DF AND NOT CONFIG_SOC_SERIES_BSIM_NRFXX)
  zephyr_library_sources(
    hal/nrf5/radio/radio_df.c
    )
endif()

zephyr_library_include_directories(
  .
  ../../../controller
  ../../../controller/ll_sw
  ../../../controller/include
  )

# This path needs to be added globally as it is supposed to be used
# in nrfx_glue.h when other libraries are built.
zephyr_include_directories(
  hal/nrf5/nrfx_glue
  )

zephyr_library_compile_options_ifdef(
  CONFIG_BT_CTLR_OPTIMIZE_FOR_SPEED
  ${OPTIMIZE_FOR_SPEED_FLAG}
  )

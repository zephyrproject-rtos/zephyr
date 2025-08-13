# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources(
  lll/lll.c
  )
if(CONFIG_BT_BROADCASTER)
  zephyr_library_sources(
    lll/lll_adv.c
    )
endif()
if(CONFIG_BT_OBSERVER)
  zephyr_library_sources(
    lll/lll_scan.c
    )
endif()
if(CONFIG_BT_CONN)
  zephyr_library_sources(
    lll/lll_clock.c
    lll/lll_conn.c
    )
  if(CONFIG_BT_PERIPHERAL)
    zephyr_library_sources(
      lll/lll_peripheral.c
      )
  endif()
  if(CONFIG_BT_CENTRAL)
    zephyr_library_sources(
      lll/lll_central.c
      )
  endif()
endif()

zephyr_library_sources_ifdef(
  CONFIG_BT_CTLR_DTM
  lll/lll_test.c
  )

zephyr_library_sources(
  hal/RV32M1/cntr.c
  hal/RV32M1/ecb.c
  hal/RV32M1/radio/radio.c
  hal/RV32M1/mayfly.c
  hal/RV32M1/ticker.c
  )

zephyr_library_include_directories(
  .
  ../../../controller
  ../../../controller/ll_sw
  ../../../controller/include
  )

zephyr_library_compile_options_ifdef(
  CONFIG_BT_CTLR_OPTIMIZE_FOR_SPEED
  ${COMPILER_OPTIMIZE_FOR_SPEED_FLAG}
  )

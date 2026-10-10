# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_link_libraries(subsys__bluetooth)

zephyr_library_sources(
  "lll/lll.c"
  "lll/lll_clock.c"
  "lll/lll_radio.c"
  "lll/lll_addr.c"
  "../lll_prepare_pipeline.c"
)

if(CONFIG_BT_BROADCASTER)
  zephyr_library_sources(
    "lll/lll_adv.c"
    "../lll_adv_pdu.c"
  )
endif()

zephyr_library_sources(
  "hal/bsim/cntr.c"
  "hal/bsim/ecb.c"
  "hal/bsim/ticker.c"
  "../../hal/mayfly.c"
  "../../hal/ticker.c"
)

zephyr_library_include_directories(
  "."
  "${ZEPHYR_BASE}/subsys/bluetooth/controller"
  "${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw"
  "${ZEPHYR_BASE}/subsys/bluetooth/controller/include"
)

# On nRF based boards, nrfx_reserved_resources.h includes
# bt_ctlr_used_resources.h in every library when the split controller is
# enabled.
zephyr_include_directories("hal/bsim/glue")

zephyr_library_compile_options_ifdef(CONFIG_BT_CTLR_OPTIMIZE_FOR_SPEED
  "${COMPILER_OPTIMIZE_FOR_SPEED_FLAG}"
)

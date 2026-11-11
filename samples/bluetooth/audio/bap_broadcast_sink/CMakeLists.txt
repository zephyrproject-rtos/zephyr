# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bap_broadcast_sink)

target_sources(app PRIVATE
  src/main.c
  src/stream_rx.c
)

target_sources_ifdef(CONFIG_ENABLE_LC3 app PRIVATE src/lc3.c)
target_sources_ifdef(CONFIG_CHAN_METRICS_PRINT app PRIVATE src/chan_metrics.c)
target_sources_ifdef(CONFIG_VS_FATAL_PRINT app PRIVATE src/vs_fatal.c)

if(CONFIG_USE_CODEC_AUDIO_OUTPUT)
  target_sources(app PRIVATE src/hw_codec.c)
endif()

if(CONFIG_USE_USB_AUDIO_OUTPUT)
  include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)
  target_sources(app PRIVATE src/usb.c)
endif()

if(CONFIG_BT_LL_SW_SPLIT AND CONFIG_CODE_DATA_RELOCATION_SRAM)
  zephyr_code_relocate(LIBRARY subsys__bluetooth__controller__ll_sw__nordic LOCATION RAM NOKEEP)

  # Controller code outside the Nordic LLL used by the radio, radio timer and LLL handlers,
  # channel selection and mayfly, and by the ticker
  set(ctlr_dir ${ZEPHYR_BASE}/subsys/bluetooth/controller)
  zephyr_code_relocate(FILES
    ${ctlr_dir}/ll_sw/lll_chan.c
    ${ctlr_dir}/util/util.c
    ${ctlr_dir}/util/mayfly.c
    ${ctlr_dir}/util/memq.c
    ${ctlr_dir}/ticker/ticker.c
    LOCATION RAM NOKEEP)
endif()

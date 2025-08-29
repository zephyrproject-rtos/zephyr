# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hap_ha_bap_broadcast_sink)

target_sources(app PRIVATE
  src/bap_unicast_sr.c
  src/has_server.c
  src/main.c
  src/vcp_vol_renderer.c
  src/stream_rx.c
)

target_sources_ifdef(CONFIG_HAP_HA_HEARING_AID_BINAURAL app PRIVATE
  src/csip_set_member.c
)

target_sources_ifdef(CONFIG_BT_ASCS_ASE_SRC app PRIVATE
  src/micp_mic_dev.c
)

target_sources_ifdef(CONFIG_BT_TBS_CLIENT app PRIVATE
  src/ccp_call_ctrl.c
)
target_sources_ifdef(CONFIG_ENABLE_LC3 app PRIVATE src/lc3.c)

if (CONFIG_USE_USB_AUDIO_OUTPUT)
  include(${ZEPHYR_BASE}/samples/subsys/usb/common/common.cmake)
  target_sources(app PRIVATE src/usb.c)
endif()

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

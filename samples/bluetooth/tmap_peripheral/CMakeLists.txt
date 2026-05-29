# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tmap_peripheral)

target_sources(app PRIVATE
  src/main.c
  src/vcp_vol_renderer.c
  src/bap_unicast_sr.c
  src/ccp_call_ctrl.c
  src/mcp_ctlr.c
)

target_sources_ifdef(CONFIG_TMAP_PERIPHERAL_DUO app PRIVATE
  src/csip_set_member.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

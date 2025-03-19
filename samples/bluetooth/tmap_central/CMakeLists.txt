# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tmap_central)

target_sources(app PRIVATE
  src/main.c
  src/mcp_server.c
  src/vcp_vol_ctlr.c
  src/ccp_call_control_server.c
  src/cap_initiator.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(classic_static_linkage)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth)
set_source_files_properties(${ZEPHYR_BASE}/subsys/bluetooth/host/l2cap.c
  DIRECTORY ${ZEPHYR_BASE}/subsys/bluetooth/host
  PROPERTIES COMPILE_DEFINITIONS
  bt_l2cap_chan_connect=real_bt_l2cap_chan_connect
)
set_source_files_properties(${ZEPHYR_BASE}/subsys/bluetooth/host/classic/l2cap_br.c
  DIRECTORY ${ZEPHYR_BASE}/subsys/bluetooth/host/classic
  PROPERTIES COMPILE_DEFINITIONS
  bt_l2cap_br_server_register=real_bt_l2cap_br_server_register
)

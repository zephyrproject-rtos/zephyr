# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(host_sco)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE "${ZEPHYR_BASE}/subsys/bluetooth")
target_link_options(app PUBLIC
  -Wl,--wrap=bt_conn_add_sco
  -Wl,--wrap=bt_conn_lookup_addr_sco
  -Wl,--wrap=bt_conn_set_state
  -Wl,--wrap=bt_conn_unref
  -Wl,--wrap=bt_hci_cmd_send_sync
  -Wl,--wrap=bt_sco_cleanup
)

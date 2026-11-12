# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(host_keys_br)

target_sources(app PRIVATE src/main.c src/hci.c)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/bluetooth)
target_link_options(app PUBLIC
  -Wl,--wrap=bt_conn_foreach
  -Wl,--wrap=bt_settings_delete_link_key
  -Wl,--wrap=bt_conn_lookup_handle
  -Wl,--wrap=bt_conn_lookup_addr_br
  -Wl,--wrap=bt_conn_set_state
  -Wl,--wrap=bt_conn_disconnect
  -Wl,--wrap=bt_conn_unref
)

#
# Copyright (c) 2026 Tondo Ltd
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(transport_bt)

file(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})

# main.c includes the transport source to exercise its static callbacks.
get_target_property(transport_sources mcumgr_transport SOURCES)
list(FILTER transport_sources EXCLUDE REGEX "(^|/)smp_bt[.]c$")
set_property(TARGET mcumgr_transport PROPERTY SOURCES "${transport_sources}")

if(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
  target_link_options(app PUBLIC
    "LINKER:--wrap=bt_conn_disconnect"
    "LINKER:--wrap=bt_conn_drop"
  )
endif()

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/mgmt/mcumgr/transport/include/mgmt/mcumgr/transport/
)

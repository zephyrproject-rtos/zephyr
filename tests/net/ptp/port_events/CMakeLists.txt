# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ptp_port_events)

set(ptp_port_src ${ZEPHYR_BASE}/subsys/net/lib/ptp/port.c)

set_source_files_properties(${ptp_port_src} PROPERTIES
  COMPILE_DEFINITIONS
  "net_if_register_timestamp_cb=fake_net_if_register_timestamp_cb;net_if_unregister_timestamp_cb=fake_net_if_unregister_timestamp_cb;net_eth_get_ptp_clock=fake_net_eth_get_ptp_clock;ptp_clock_get=fake_ptp_clock_get;net_if_oper_state=fake_net_if_oper_state"
)

target_sources(app PRIVATE
  src/main.c
  ${ptp_port_src}
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/ptp
)

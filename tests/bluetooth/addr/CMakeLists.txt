# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

# Include Bluetooth subsystem headers for testing
target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth
)

target_sources(app PRIVATE
  src/test_bt_addr_le_eq.c
  src/test_bt_addr_from_str.c
  src/test_bt_addr_le_is_resolved.c
  src/test_bt_addr_le_from_str.c
  src/test_bt_addr_le_to_str.c
  src/test_bt_addr_matches_irk.c
)

# Resolving an RPA through the Host key store needs SMP and privacy
if(CONFIG_BT_PRIVACY AND CONFIG_BT_SMP)
  target_sources(app PRIVATE src/test_bt_addr_le_resolve.c)
endif()

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)

# Include Bluetooth subsystem headers for testing
target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth
)

target_sources(app PRIVATE
  src/test_bt_addr_le_eq.c
  src/test_bt_addr_from_str.c
  src/test_bt_addr_le_resolve.c
  src/test_bt_addr_le_from_str.c
)

target_sources_ifndef(CONFIG_BT_ADDR_LE_LEGACY_STRING_OUTPUT app PRIVATE
  src/test_bt_addr_le_to_str.c
)

target_sources_ifdef(CONFIG_BT_ADDR_LE_LEGACY_STRING_OUTPUT app PRIVATE
  src/test_bt_addr_le_to_str_legacy.c
)

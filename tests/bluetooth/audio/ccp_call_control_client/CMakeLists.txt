# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_ccp_client)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth/

  ${ZEPHYR_BASE}/tests/bluetooth/audio/ccp_call_control_client/include
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/include
)

target_sources(app PRIVATE
  # Test source files
  src/main.c
  src/test_common.c
  src/test_procedures.c

  # UUT files
  uut/ccp_call_control_client.c
  uut/tbs_client.c

  # Stack source file
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/ccp_call_control_client.c

  # Mock files
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/conn.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/gatt.c
)

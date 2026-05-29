# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bluetooth_cap_commander)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth/

  ${ZEPHYR_BASE}/tests/bluetooth/audio/cap_commander/include
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/include
)

target_sources(app PRIVATE
  # Test source files
  src/main.c
  src/test_common.c
  src/test_vcp.c
  src/test_micp.c
  src/test_broadcast_reception.c
  src/test_distribute_broadcast_code.c
  src/test_cancel.c

  # UUT files
  uut/bap_broadcast_assistant.c
  uut/aics.c
  uut/cap_commander.c
  uut/csip.c
  uut/micp.c
  uut/vcp.c
  uut/vocs.c

  # Stack source file
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/audio.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_commander.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_common.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/addr.c

  # Mock files
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/conn.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/gatt.c
)

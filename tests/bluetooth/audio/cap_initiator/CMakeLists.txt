# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bluetooth_cap_initiator)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth/

  ${ZEPHYR_BASE}/tests/bluetooth/audio/cap_initiator/include
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/include
)

target_sources(app PRIVATE
  # Test source files
  src/main.c
  src/test_common.c
  src/test_unicast_group.c
  src/test_unicast_start.c
  src/test_unicast_stop.c

  # UUT files
  uut/bap_unicast_client.c
  uut/cap_initiator.c
  uut/csip.c

  # Stack source file
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/audio.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_iso.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_stream.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_initiator.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_common.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_stream.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/ccid.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
  ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c

  # Mock files
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/conn.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/gatt.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/iso.c
)

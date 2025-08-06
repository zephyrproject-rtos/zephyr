# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_cap_handover)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth/

  ${ZEPHYR_BASE}/tests/bluetooth/audio/cap_handover/include
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/include
)

target_sources(app PRIVATE
  # Test source files
  src/broadcast_to_unicast.c
  src/unicast_to_broadcast.c
  src/test_common.c

  # UUT files
  uut/bap_broadcast_assistant.c
  uut/cap_handover.c
  uut/cap_initiator.c
  uut/csip.c

  # Stack source file
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/audio.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/codec.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_broadcast_source.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_iso.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_stream.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_commander.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_handover.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_initiator.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_common.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_stream.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/ccid.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/addr.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
  ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c

  # Mock files
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/adv.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/bap_unicast_client.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/conn.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/gatt.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/iso.c
)

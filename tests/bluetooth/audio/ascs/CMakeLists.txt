# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bluetooth_ascs)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth/

  ${ZEPHYR_BASE}/tests/bluetooth/audio/ascs/include
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/include
  )

target_sources(app PRIVATE
  # Test source files
  src/main.c
  src/test_ase_control_params.c
  src/test_ase_register.c
  src/test_ase_state_transition_invalid.c
  src/test_ase_state_transition.c
  src/test_common.c

  # UUT files
  uut/bap_unicast_client.c
  uut/bap_unicast_server.c

  # Stack source file
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/ascs.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/audio.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_iso.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_stream.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_unicast_server.c
  ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c

  # Mock files
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/bap_stream.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/conn.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/gatt.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/iso.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/pacs.c
)

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(bluetooth_bap_broadcast_source)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/include)

target_sources(app PRIVATE
  # Test source files
  src/main.c
  src/test_callback_register.c

  # Stack source file
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/audio.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_iso.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_stream.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_broadcast_source.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/codec.c

  # Mock files
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/bap_broadcast_source.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/bap_stream.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/conn.c
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/iso.c
)

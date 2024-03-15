#
# Copyright (c) 2023 Codecoup
#
# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
  src/bap_stream.c
  src/bap_unicast_client.c
  src/bap_unicast_server.c
  src/cap_commander.c
  src/conn.c
  src/crypto.c
  src/fatal.c
  src/gatt.c
  src/iso.c
  src/kernel.c
  src/mem_slab.c
  src/net_buf.c
  src/pacs.c
)

target_include_directories(mocks PUBLIC
  include
  ${ZEPHYR_BASE}/tests/bluetooth/audio
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/audio
)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)

target_link_libraries(mocks PRIVATE test_interface host_mocks)
target_link_options(mocks PUBLIC
  "SHELL:-T ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/mock-sections.ld")

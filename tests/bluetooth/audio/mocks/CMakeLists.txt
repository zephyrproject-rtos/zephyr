#
# Copyright (c) 2023 Codecoup
# Coperight (c) 2024 Demant A/S
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
  src/bap_stream.c
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
  ${ZEPHYR_BASE}/subsys/bluetooth/common
  ${ZEPHYR_BASE}/include/zephyr
)

target_sources(testbinary PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth/common/addr.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
  ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c
  ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
  ${ZEPHYR_BASE}/include/zephyr/kernel.h
)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/host host_mocks)

target_link_libraries(mocks PRIVATE test_interface host_mocks)
target_link_options(mocks PUBLIC
  "SHELL:-T ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/mock-sections.ld")

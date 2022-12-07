#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt file for creating of mocks library.
#

add_library(mocks STATIC
  src/kernel.c
  src/ecb.c
  src/mayfly.c
  src/lll.c
  src/lll_conn.c
  src/ll_assert.c
  src/assert.c
  src/util.c
  src/ticker.c
  src/ull.c
  src/ull_conn_iso.c
  src/ull_peripheral.c
  src/ull_peripheral_iso.c
  src/ull_central.c
  src/ull_scan.c
  src/lll_clock.c
)


target_include_directories(mocks PUBLIC
  include
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/include
  ${ZEPHYR_BASE}/subsys/bluetooth/controller
  ${ZEPHYR_BASE}/subsys/bluetooth
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw
  ${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
  ${ZEPHYR_BASE}/include/zephyr/bluetooth
)

target_link_libraries(mocks PRIVATE test_interface)

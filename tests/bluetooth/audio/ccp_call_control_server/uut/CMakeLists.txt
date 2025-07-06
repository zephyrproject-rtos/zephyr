#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt file for creating of uut library.
#

add_library(uut STATIC
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/audio.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/ccid.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/ccp_call_control_server.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/tbs.c
  ${ZEPHYR_BASE}/lib/net_buf/buf_simple.c
  ${ZEPHYR_BASE}/lib/utils/utf8.c
)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/audio/mocks mocks)

target_link_libraries(uut PUBLIC test_interface mocks)

target_compile_options(uut PRIVATE -std=c11 -include ztest.h)

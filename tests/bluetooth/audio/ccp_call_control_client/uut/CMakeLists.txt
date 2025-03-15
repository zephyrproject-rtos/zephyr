#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt file for creating of uut library.
#

add_library(uut STATIC
  ${ZEPHYR_BASE}/lib/net_buf/buf_simple.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/ccp_call_control_client.c
  ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
  ccp_call_control_client.c
  tbs_client.c
)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/audio/mocks mocks)

target_link_libraries(uut PUBLIC test_interface mocks)
target_include_directories(uut PRIVATE ${ZEPHYR_BASE}/tests/bluetooth/audio/ccp_call_control_client/include)

target_compile_options(uut PRIVATE -std=c11 -include ztest.h)

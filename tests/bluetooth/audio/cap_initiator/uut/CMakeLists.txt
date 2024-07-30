#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt file for creating of uut library.
#

add_library(uut STATIC
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/audio.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_iso.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_stream.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_initiator.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_common.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/cap_stream.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
  ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
  ${ZEPHYR_BASE}/subsys/net/buf_simple.c
  bap_unicast_client.c
  cap_initiator.c
  csip.c
)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/audio/mocks mocks)

target_link_libraries(uut PUBLIC test_interface mocks)
target_include_directories(uut PRIVATE ${ZEPHYR_BASE}/tests/bluetooth/audio/cap_initiator/include)

target_compile_options(uut PRIVATE -std=c11 -include ztest.h)

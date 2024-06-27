#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#
# CMakeLists.txt file for creating of uut library.
#

add_library(uut STATIC
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/ascs.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/audio.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_iso.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_stream.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/bap_unicast_server.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
  ${ZEPHYR_BASE}/subsys/bluetooth/host/data.c
  ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c
  ${ZEPHYR_BASE}/subsys/logging/log_minimal.c
  ${ZEPHYR_BASE}/subsys/net/buf_simple.c
  bap_unicast_client.c
  bap_unicast_server.c
)

add_subdirectory(${ZEPHYR_BASE}/tests/bluetooth/audio/mocks mocks)

target_link_libraries(uut PUBLIC test_interface mocks)
target_include_directories(uut PRIVATE ${ZEPHYR_BASE}/tests/bluetooth/audio/ascs/include)

target_compile_options(uut PRIVATE -std=c11 -include ztest.h)

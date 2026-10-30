# SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_vocs_client)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/subsys/bluetooth/
  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/include
)

target_sources(app PRIVATE
  src/main.c

  ${ZEPHYR_BASE}/subsys/bluetooth/audio/vocs.c
  ${ZEPHYR_BASE}/subsys/bluetooth/audio/vocs_client.c
  ${ZEPHYR_BASE}/subsys/bluetooth/common/bt_str.c
  ${ZEPHYR_BASE}/subsys/bluetooth/host/uuid.c
  ${ZEPHYR_BASE}/lib/uuid/uuid.c

  ${ZEPHYR_BASE}/tests/bluetooth/audio/mocks/src/conn.c
)

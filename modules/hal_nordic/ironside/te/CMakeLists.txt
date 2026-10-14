# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(${ZEPHYR_IRONSIDE_INTEGRATION_KIT_MODULE_DIR}/client/include)
zephyr_include_directories(include)

zephyr_library()
zephyr_library_sources(
  ${ZEPHYR_IRONSIDE_INTEGRATION_KIT_MODULE_DIR}/client/src/client.c
  ${ZEPHYR_IRONSIDE_INTEGRATION_KIT_MODULE_DIR}/client/src/services.c
  client_lock.c
  layout_check.c
)

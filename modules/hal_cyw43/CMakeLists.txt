# Copyright (c) 2026 Igalia S.L.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_CYW43_DRIVER)
  zephyr_include_directories(
    ${ZEPHYR_HAL_CYW43_MODULE_DIR}
    ${ZEPHYR_HAL_CYW43_MODULE_DIR}/src
  )
endif()

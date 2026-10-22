# Copyright (c) 2025 Endress+Hauser GmbH+Co. KG
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_DISK_DRIVER_FTL)
  zephyr_include_directories(${ZEPHYR_CURRENT_MODULE_DIR}/include)

  file(GLOB dhara_sources ${ZEPHYR_CURRENT_MODULE_DIR}/dhara/*.c)

  zephyr_library()
  zephyr_library_sources(
    ${dhara_sources}
  )
endif()

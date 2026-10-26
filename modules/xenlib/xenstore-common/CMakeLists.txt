# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2025 TOKITA Hiroshi

add_library(XEN_STORE_COMMON INTERFACE)

target_include_directories(XEN_STORE_COMMON INTERFACE
  ${ZEPHYR_XENLIB_MODULE_DIR}/xenstore-common/include)

zephyr_library()
zephyr_library_sources(
  ${ZEPHYR_XENLIB_MODULE_DIR}/xenstore-common/src/xenstore_common.c)
zephyr_library_link_libraries(XEN_STORE_COMMON)
zephyr_include_directories(${ZEPHYR_XENLIB_MODULE_DIR}/xenstore-common/include)

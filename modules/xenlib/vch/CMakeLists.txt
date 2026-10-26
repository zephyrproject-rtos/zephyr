# Copyright (c) 2023 EPAM Systems
#
# SPDX-License-Identifier: Apache-2.0
#

add_library(VCH INTERFACE)
target_include_directories(VCH INTERFACE 
  ${ZEPHYR_XENLIB_MODULE_DIR}/vch/include)

zephyr_library()
zephyr_library_sources(${ZEPHYR_XENLIB_MODULE_DIR}/vch/src/vch.c)
zephyr_library_link_libraries(VCH)
zephyr_include_directories(${ZEPHYR_XENLIB_MODULE_DIR}/vch/include)

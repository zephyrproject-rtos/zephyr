# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
include(${ZEPHYR_BASE}/cmake/modules/app_version.cmake)
configure_file(${ZEPHYR_BASE}/app_version.h.in ${OUT_FILE})

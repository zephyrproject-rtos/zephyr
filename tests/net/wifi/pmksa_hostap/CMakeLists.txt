# Copyright (c) 2026 Siddhant Modi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_pmksa_hostap)

if(NOT ZEPHYR_HOSTAP_MODULE_DIR)
  message(FATAL_ERROR "The hostap module is required by this test")
endif()

zephyr_include_directories(
  ${ZEPHYR_BASE}/modules/hostap/src
  ${ZEPHYR_HOSTAP_MODULE_DIR}
  ${ZEPHYR_HOSTAP_MODULE_DIR}/wpa_supplicant
  ${ZEPHYR_HOSTAP_MODULE_DIR}/src
  ${ZEPHYR_HOSTAP_MODULE_DIR}/src/utils
  ${ZEPHYR_HOSTAP_MODULE_DIR}/src/common
  ${ZEPHYR_HOSTAP_MODULE_DIR}/src/rsn_supp
)

target_sources(app PRIVATE src/main.c)

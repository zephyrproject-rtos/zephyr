#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi_credentials_test)

if(CONFIG_WIFI_CREDENTIALS)
  # The library is built with the backend selected in Kconfig.
  target_sources(app PRIVATE src/backend_psa.c)
  return()
endif()

target_sources(app PRIVATE src/main.c)

target_sources(app
  PRIVATE
  ${ZEPHYR_BASE}/subsys/net/lib/wifi_credentials/wifi_credentials.c
)

zephyr_include_directories(${ZEPHYR_BASE}/subsys/testsuite/include)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/net/lib/wifi_credentials/)

target_compile_options(app
  PRIVATE
  -DCONFIG_WIFI_CREDENTIALS_MAX_ENTRIES=2
  -DCONFIG_WIFI_CREDENTIALS_SAE_PASSWORD_LENGTH=128
  -DCONFIG_WIFI_CREDENTIALS_LOG_LEVEL=4
)

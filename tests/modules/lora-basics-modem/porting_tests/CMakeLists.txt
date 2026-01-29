# Copyright (c) 2025 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(lbm_porting LANGUAGES C)

set(LBM_DIR ${ZEPHYR_LORA_BASICS_MODEM_MODULE_DIR}/lbm_lib)

# Include LBM internal headers needed for porting tests
target_include_directories(app PRIVATE
  ${LBM_DIR}/smtc_modem_hal
  ${ZEPHYR_BASE}/modules/lora-basics-modem/smtc_modem_hal
  ${LBM_DIR}/smtc_modem_core/smtc_ral/src
  ${LBM_DIR}/smtc_modem_core/smtc_ralf/src
  ${LBM_DIR}/smtc_modem_core/radio_drivers/sx126x_driver/src
)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

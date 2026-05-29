# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(renesas_ctsu)

target_sources(app PRIVATE src/main.c)

if(CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG)
  zephyr_include_directories(src/qe_generated_files)
  zephyr_sources(src/qe_generated_files/qe_touch_config.c)
endif()

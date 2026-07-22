# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(coproc_launcher)

if(CONFIG_NRF_REMOTE_IMAGE_INC_DIR)
  zephyr_include_directories(${CONFIG_NRF_REMOTE_IMAGE_INC_DIR})
endif()

target_sources(app PRIVATE src/main.c)

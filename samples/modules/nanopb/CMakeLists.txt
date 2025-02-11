# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(nanopb_sample)

list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
include(nanopb)

if(CONFIG_SAMPLE_UNLUCKY_NUMBER)
  set(unlucky_number_type "FT_DEFAULT")
else()
  set(unlucky_number_type "FT_IGNORE")
endif()
zephyr_nanopb_sources(app src/simple.proto)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

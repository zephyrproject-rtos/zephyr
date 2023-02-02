# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
include_directories("./src")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_ctrl_user_ext)

zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/bluetooth/controller/)

if(CONFIG_SOC_COMPATIBLE_NRF)
  zephyr_library_include_directories(
    $ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
    $ENV{ZEPHYR_BASE}/subsys/bluetooth/hci/nordic
  )
elseif(CONFIG_SOC_OPENISA_RV32M1_RISCV32)
  zephyr_library_include_directories(
    $ENV{ZEPHYR_BASE}/bluetooth/controller/ll_sw/openisa
    $ENV{ZEPHYR_BASE}/bluetooth/hci/openisa
  )
endif()


FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})

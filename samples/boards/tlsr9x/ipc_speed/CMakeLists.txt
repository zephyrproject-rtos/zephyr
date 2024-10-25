# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipc-speed)

if (NOT CONFIG_SOC_RISCV_TELINK_W91)
  message(FATAL_ERROR "SOC is not supported for this sample")
endif()

target_sources(app PRIVATE src/main.c)

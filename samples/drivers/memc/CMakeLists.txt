# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(memc)


if(CONFIG_MEMC_MCUX_FLEXSPI)
  target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers/memc)
endif()

target_sources(app PRIVATE src/main.c)

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wireguard_vpn)

zephyr_library_include_directories(${ZEPHYR_WIREGUARD_MODULE_DIR}/include)

target_sources(app PRIVATE
  src/main.c
  src/wg.c
)

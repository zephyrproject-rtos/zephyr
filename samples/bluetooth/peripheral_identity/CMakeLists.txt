# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(peripheral_identity)

target_sources(app PRIVATE
  src/main.c
  src/peripheral_identity.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

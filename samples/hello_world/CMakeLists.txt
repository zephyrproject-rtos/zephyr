# SPDX-License-Identifier: Agache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

target_sources(app PRIVATE src/main.c)

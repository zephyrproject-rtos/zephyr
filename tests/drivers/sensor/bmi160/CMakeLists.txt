# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device)

target_include_directories(app PRIVATE include)
target_sources(app
  PRIVATE
    include/checks.h
    include/fixture.h
    src/fixture.c
    src/i2c.c
    src/spi.c
)

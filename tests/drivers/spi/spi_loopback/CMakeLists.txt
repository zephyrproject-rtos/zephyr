# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

# Suppress "unique_unit_address_if_enabled" to handle fast and slow loopback overlaps:
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(spi_loopback)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

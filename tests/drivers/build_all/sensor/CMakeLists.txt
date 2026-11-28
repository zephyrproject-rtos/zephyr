# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

# Ignore duplicate addresses on buses
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(build_all)

# Exclude main when running the generic test because ztest supplies its own
target_sources_ifndef(CONFIG_GENERIC_SENSOR_TEST app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_GENERIC_SENSOR_TEST app PRIVATE src/generic_test.c)

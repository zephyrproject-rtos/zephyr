# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(device)

target_sources(app PRIVATE src/test_sbs_gauge.c)
target_sources_ifndef(CONFIG_TEST_SBS_CUTOFF_EXTENSION app PRIVATE src/test_cutoff_disabled.c)
target_sources_ifdef(CONFIG_TEST_SBS_CUTOFF_EXTENSION app PRIVATE src/test_cutoff.c)

target_include_directories(app PRIVATE include)

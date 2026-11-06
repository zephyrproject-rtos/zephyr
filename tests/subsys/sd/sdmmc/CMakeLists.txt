# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sdmmc_subsys_test)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_PM app PRIVATE src/test_sdmmc_pm.c)

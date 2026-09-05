# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bq41z50)

file(GLOB app_sources src/test_bq41z50.c)
target_sources(app PRIVATE ${app_sources})

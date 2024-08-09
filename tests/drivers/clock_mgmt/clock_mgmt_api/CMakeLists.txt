# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(clock_mgmt_api)

FILE(GLOB app_sources src/*.c)
FILE(GLOB clock_sources src/clock_drivers/*.c)
target_sources(app PRIVATE ${app_sources} ${clock_sources})

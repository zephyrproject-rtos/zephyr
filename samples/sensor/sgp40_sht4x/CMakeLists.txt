# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sgp40_sht4x)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

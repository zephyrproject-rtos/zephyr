# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ipm_imx)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/drivers)
target_sources(app PRIVATE src/main.c)

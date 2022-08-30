# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(log_syst)

zephyr_include_directories(
${ZEPHYR_BASE}/tests/subsys/logging/log_syst/src
)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

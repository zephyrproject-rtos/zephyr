# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(SHIELD adi_ev_ade9153ashieldz)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ade9153a)

include_directories("${ZEPHYR_BASE}/drivers/sensor/adi/ade9153a")

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

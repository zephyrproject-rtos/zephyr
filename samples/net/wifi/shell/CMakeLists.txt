# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)


set(EXTRA_CONF_FILE "${CMAKE_CURRENT_SOURCE_DIR}/overlay-p2p.conf" CACHE STRING "" FORCE)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(wifi)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

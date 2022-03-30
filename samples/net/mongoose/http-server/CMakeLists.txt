# Copyright (c) 2022 Cesanta Software Limited
# Mail: support@cesanta.com
#
# SPDX-License-Identifier: GPL2.0 or commercial

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mongoose_http_server)

#add_definitions(-DMG_ENABLE_LINES=1)
add_definitions(-DMG_ENABLE_SSI=0)
add_definitions(-I${CMAKE_CURRENT_SOURCE_DIR}/../common)
add_definitions(-g3 -O0)

target_sources(app PRIVATE
	src/main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../common/mongoose.c
)

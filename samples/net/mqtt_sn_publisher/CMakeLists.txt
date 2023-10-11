# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(mqtt_sn_publisher)

target_sources(app PRIVATE
	src/main.c
	src/udp.c
)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

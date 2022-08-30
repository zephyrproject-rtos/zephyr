# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
include_directories("./src")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_ctrl_sw_privacy)

zephyr_library_include_directories(
	${ZEPHYR_BASE}/subsys/bluetooth
	${ZEPHYR_BASE}/subsys/bluetooth/controller
	${ZEPHYR_BASE}/subsys/bluetooth/controller/include
	${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
	${ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic/lll
)

FILE(GLOB app_sources src/*.c)

target_sources(app PRIVATE ${app_sources})

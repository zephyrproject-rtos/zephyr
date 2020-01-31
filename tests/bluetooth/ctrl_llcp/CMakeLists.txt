# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
include_directories("./src")

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(bluetooth_ctrl_llcp)

zephyr_library_include_directories(
	$ENV{ZEPHYR_BASE}/subsys/bluetooth
	$ENV{ZEPHYR_BASE}/subsys/bluetooth/controller
	$ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/include
	$ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw
	$ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/nordic
)

FILE(GLOB app_sources src/*.c)

FILE(GLOB ll_sw_sources
  $ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/util/mem.c
  $ENV{ZEPHYR_BASE}/subsys/bluetooth/controller/ll_sw/ull_tx_queue.c
)

target_sources(app PRIVATE ${app_sources} ${ll_sw_sources})

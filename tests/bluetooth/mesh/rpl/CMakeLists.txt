# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_mesh_rpl)

FILE(GLOB app_sources src/*.c)
target_sources(app
	PRIVATE
	${app_sources}
	${ZEPHYR_BASE}/subsys/bluetooth/mesh/rpl.c)

target_include_directories(app
	PRIVATE
	${ZEPHYR_BASE}/subsys/bluetooth/mesh)

target_compile_options(app
	PRIVATE
	-DCONFIG_BT_MESH_CRPL=10
	-DCONFIG_BT_MESH_RPL_STORE_TIMEOUT=1
	-DCONFIG_BT_SETTINGS)

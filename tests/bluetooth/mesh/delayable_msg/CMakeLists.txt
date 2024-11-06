# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bluetooth_mesh_delayable_msg)

FILE(GLOB app_sources src/*.c)
target_sources(app
	PRIVATE
	${app_sources}
	${ZEPHYR_BASE}/subsys/bluetooth/mesh/delayable_msg.c)

target_include_directories(app
	PRIVATE
	${ZEPHYR_BASE}/subsys/bluetooth/mesh
	${ZEPHYR_MBEDTLS_MODULE_DIR}/include)

target_compile_options(app
	PRIVATE
	-DCONFIG_BT_MESH_TX_SEG_MAX=32
	-DCONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_COUNT=4
	-DCONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE=20
	-DCONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT=20
	-DCONFIG_BT_MESH_USES_MBEDTLS_PSA)

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ocpp)

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)
target_sources(app PRIVATE src/main.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
	app
	src/credentials/ec.der
	${gen_dir}/ec.der.inc
)

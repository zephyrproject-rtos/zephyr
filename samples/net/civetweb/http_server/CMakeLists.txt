# Copyright (c) 2020 Alexander Kozhinov
# Mail: AlexanderKozhinov@yandex.com
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(common_dir ${CMAKE_CURRENT_SOURCE_DIR}/../common)
set(common_src_dir ${common_dir}/src)
set(common_include_dir ${common_dir}/include)

include_directories(
	${common_include_dir}
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(civetweb_http_server)

target_sources(app PRIVATE
	src/main.c
	${common_src_dir}/libc_extensions.c
)

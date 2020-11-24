# Copyright (c) 2020 Alexander Kozhinov <AlexanderKozhinov@yandex.com>
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(common_dir ${CMAKE_CURRENT_SOURCE_DIR}/../common)
set(common_src_dir ${common_dir}/src)
set(common_include_dir ${common_dir}/include)

set(inc_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(src_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)

include_directories(
	${inc_dir}
	${common_include_dir}
)

option(CIVETWEB_ENABLE_WEBSOCKETS "Enable websockets connections" ON)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(civetweb_websocket_server)

target_sources(app PRIVATE
		${src_dir}/main.c
		${common_src_dir}/libc_extensions.c
		${src_dir}/http_server_handlers.c
		${src_dir}/websocket_server_handlers.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
set(web_page_dir web_page)

# List of files that are used to generate .h file that can be included
# into .c file.
foreach(inc_file
	${web_page_dir}/index.html
	${web_page_dir}/index.css
	${web_page_dir}/ws.js)

	# Generate normal and gzipped versions of each file:
	generate_inc_file_for_target(app
		${inc_file}
		${gen_dir}/${inc_file}.inc)

	generate_inc_file_for_target(app
		${inc_file}
		${gen_dir}/${inc_file}.gz.inc
		--gzip)

endforeach()

include(${ZEPHYR_BASE}/samples/net/common/common.cmake)

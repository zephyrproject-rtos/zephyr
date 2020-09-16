# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(civetweb)

target_sources(app PRIVATE src/main.c src/libc_extensions.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
set(web_page_dir web_page)

# List of files that are used to generate .h file that can be included
# into .c file.
foreach(inc_file
	${web_page_dir}/jquery-3.5.1.slim.min.js)

	# Generate normal and gzipped versions of each file:
	generate_inc_file_for_target(app
		${inc_file}
		${gen_dir}/${inc_file}.inc)

	generate_inc_file_for_target(app
		${inc_file}
		${gen_dir}/${inc_file}.gz.inc
		--gzip)

endforeach()

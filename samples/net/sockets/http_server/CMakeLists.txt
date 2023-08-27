# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
find_package(Python REQUIRED COMPONENTS Interpreter)

project(http_server)

option(INCLUDE_HTML_CONTENT "Include the HTML content" ON)

target_sources(app PRIVATE src/main.c)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

set(source_file_index src/index.html)
generate_inc_file_for_target(app ${source_file_index} ${gen_dir}/index.html.gz.inc --gzip)

set(source_file_not_found src/not_found_page.html)
generate_inc_file_for_target(app ${source_file_not_found} ${gen_dir}/not_found_page.html.gz.inc --gzip)

target_link_libraries(app PRIVATE zephyr_interface zephyr)

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_iterable_section(NAME http_resource_desc_test_http_service KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)

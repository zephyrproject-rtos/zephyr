# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(crime)

set(BASE_PATH "../../../../../subsys/net/lib/http/")
include_directories(${BASE_PATH}/headers)

FILE(GLOB app_sources src/main.c)
target_sources(app PRIVATE ${app_sources})

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

set(source_file_index src/index.html)
generate_inc_file_for_target(app ${source_file_index} ${gen_dir}/index.html.inc)
generate_inc_file_for_target(app ${source_file_index} ${gen_dir}/index.html.gz.inc --gzip)

set(source_file_not_found src/not_found_page.html)
generate_inc_file_for_target(app ${source_file_not_found} ${gen_dir}/not_found_page.html.inc)
generate_inc_file_for_target(app ${source_file_not_found} ${gen_dir}/not_found_page.html.gz.inc --gzip)

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_iterable_section(NAME http_resource_desc_test_http_service KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN 4)

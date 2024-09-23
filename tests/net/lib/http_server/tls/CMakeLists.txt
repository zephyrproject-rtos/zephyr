# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tls)

set(BASE_PATH "../../../../../subsys/net/lib/http/")
include_directories(${BASE_PATH}/headers)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

if (${CONFIG_TLS_CREDENTIALS})
  generate_inc_file_for_target(
    app
    ${ZEPHYR_BASE}/samples/net/sockets/http_server/src/ca.der
    ${gen_dir}/ca.inc
  )

  generate_inc_file_for_target(
    app
    ${ZEPHYR_BASE}/samples/net/sockets/http_server/src/server.der
    ${gen_dir}/server.inc
  )

  generate_inc_file_for_target(
    app
    ${ZEPHYR_BASE}/samples/net/sockets/http_server/src/server_privkey.der
    ${gen_dir}/server_privkey.inc
  )

  # we reuse the same certificate / private key for client
  # since it seems to be the only one that is signed by a ca
  generate_inc_file_for_target(
    app
    ${ZEPHYR_BASE}/samples/net/sockets/http_server/src/server.der
    ${gen_dir}/client.inc
  )

  generate_inc_file_for_target(
    app
    ${ZEPHYR_BASE}/samples/net/sockets/http_server/src/server_privkey.der
    ${gen_dir}/client_privkey.inc
  )
endif()

set(source_file_index src/index.html)
generate_inc_file_for_target(app ${source_file_index} ${gen_dir}/index.html.inc)
generate_inc_file_for_target(app ${source_file_index} ${gen_dir}/index.html.gz.inc --gzip)

FILE(GLOB app_sources src/main.c)
target_sources(app PRIVATE ${app_sources})

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_iterable_section(NAME http_resource_desc_test_http_service KVMA RAM_REGION GROUP RODATA_REGION SUBALIGN ${CONFIG_LINKER_ITERABLE_SUBALIGN})

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(http_client_multiprotocol_http2)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

if(CONFIG_TLS_CREDENTIALS)
  set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
  set(certs_dir ${ZEPHYR_BASE}/samples/net/sockets/http_server/src/certs)

  generate_inc_file_for_target(app ${certs_dir}/ca_cert.der ${gen_dir}/ca.inc)
  generate_inc_file_for_target(app ${certs_dir}/server_cert.der ${gen_dir}/server.inc)
  generate_inc_file_for_target(app ${certs_dir}/server_privkey.der ${gen_dir}/server_privkey.inc)
endif()

zephyr_linker_sources(SECTIONS sections-rom.ld)
zephyr_iterable_section(NAME http_resource_desc_test_http_service KVMA RAM_REGION GROUP RODATA_REGION)

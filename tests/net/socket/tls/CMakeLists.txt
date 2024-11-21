# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(socket_tls)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

generate_inc_file_for_target(
  app
  ${ZEPHYR_BASE}/samples/net/sockets/echo_server/src/ca.der
  ${gen_dir}/ca.inc
)

generate_inc_file_for_target(
  app
  ${ZEPHYR_BASE}/samples/net/sockets/echo_server/src/server.der
  ${gen_dir}/server.inc
)

generate_inc_file_for_target(
  app
  ${ZEPHYR_BASE}/samples/net/sockets/echo_server/src/server_privkey.der
  ${gen_dir}/server_privkey.inc
)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/sockets)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

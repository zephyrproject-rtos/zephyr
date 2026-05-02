# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(socket_quic)

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/ip)
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/net/lib/quic)
FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

if(CONFIG_QUIC)
  foreach(inc_file
    ca.crt
    server.crt
    server.key
    client.crt
    client.key
  )
    generate_inc_file_for_target(app
      ${ZEPHYR_BASE}/tests/net/lib/quic/certs/${inc_file}
      ${gen_dir}/${inc_file}.inc
    )
  endforeach()

  target_include_directories(app PRIVATE ${ZEPHYR_BASE}/tests/net/lib/quic/certs/)
endif()

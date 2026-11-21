# SPDX-FileCopyrightText: 2026 Muhammad Waleed Badar
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(transport_udp_dtls)

target_sources(app PRIVATE src/main.c)

set(cert_dir ${ZEPHYR_BASE}/samples/net/sockets/echo_server/src)
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated)

foreach(cert_file echo-apps-cert.der echo-apps-key.der)
  generate_inc_file_for_target(app ${cert_dir}/${cert_file} ${gen_dir}/${cert_file}.inc)
endforeach()

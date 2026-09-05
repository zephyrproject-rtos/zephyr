# Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fota_http_test)

target_sources(app PRIVATE src/main.c)

zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)

if(CONFIG_FOTA_HTTP_TLS)
  set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
  set(cert_dir ${ZEPHYR_BASE}/samples/net/sockets/echo_server/src)

  foreach(inc_file ca.der server.der server_privkey.der)
    generate_inc_file_for_target(app ${cert_dir}/${inc_file} ${gen_dir}/${inc_file}.inc)
  endforeach()
endif()

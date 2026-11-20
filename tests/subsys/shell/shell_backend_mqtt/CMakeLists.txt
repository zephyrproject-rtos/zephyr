# Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(shell_backend_mqtt)

target_sources(app PRIVATE src/main.c)
zephyr_ld_options(
  -Wl,--wrap=mqtt_abort
  -Wl,--wrap=mqtt_connect
  -Wl,--wrap=mqtt_read_publish_payload_blocking
)

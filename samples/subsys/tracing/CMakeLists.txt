# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(BOARD MATCHES "qemu_.*")
  list(APPEND QEMU_EXTRA_FLAGS -serial file:channel0_0)
endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tracing_tests)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_TRACING_GPIO app PRIVATE src/gpio_main.c)
target_sources_ifdef(CONFIG_TRACING_USER app PRIVATE src/tracing_user.c)

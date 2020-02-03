# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

if(BOARD MATCHES "qemu_.*")
  list(APPEND QEMU_EXTRA_FLAGS -serial file:channel0_0)
endif()

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(tracing_tests)

target_sources(app PRIVATE src/main.c)

target_sources_ifdef(
  CONFIG_TRACING_TEST
  app PRIVATE
  src/tracing_string_format_test.c
)

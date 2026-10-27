# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS "$ENV{ZEPHYR_BASE}")
project(thread_restart)

target_sources_ifdef(CONFIG_THREAD_RESTARTABLE app PRIVATE
  src/main.c
)
target_sources_ifndef(CONFIG_THREAD_RESTARTABLE app PRIVATE
  src/disabled.c
)

target_include_directories(app PRIVATE
  ${ZEPHYR_BASE}/kernel/include
  ${ZEPHYR_BASE}/arch/${ARCH}/include
)

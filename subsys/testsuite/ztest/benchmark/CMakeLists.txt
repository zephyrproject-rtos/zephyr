# Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.28)
zephyr_library_sources_ifdef(CONFIG_ZTEST_BENCHMARK
  ${CMAKE_CURRENT_LIST_DIR}/src/benchmark.c
)
zephyr_linker_sources(DATA_SECTIONS benchmark.ld)

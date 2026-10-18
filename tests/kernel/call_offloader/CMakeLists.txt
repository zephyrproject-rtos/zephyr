# Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(call_offloader_engine)

if(CONFIG_ZTEST_BENCHMARK)
  target_sources(app PRIVATE src/dispatch.c)
elseif(CONFIG_ZTEST_FATAL_HOOK)
  target_sources(app PRIVATE src/fault.c)
else()
  target_sources(app PRIVATE src/main.c src/wrap.c src/init.c)
endif()

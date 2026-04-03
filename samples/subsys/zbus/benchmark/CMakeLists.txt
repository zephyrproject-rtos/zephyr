# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(benchmark)

set(app_sources src/benchmark.c)

if(CONFIG_BM_LISTENERS)
  list(APPEND app_sources src/lis.c)

elseif(CONFIG_BM_SUBSCRIBERS)
  list(APPEND app_sources src/sub.c)

elseif(CONFIG_BM_MSG_SUBSCRIBERS)
  list(APPEND app_sources src/msg_sub.c)

endif()

target_sources(app PRIVATE ${app_sources})

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(heap_stats_log)

file(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# White-box access to the internal heap header (heap.h).
# The test intentionally inspects struct z_heap internals to verify the
# stats framework without a dedicated getter API.
target_include_directories(app PRIVATE ${ZEPHYR_BASE}/lib/heap)

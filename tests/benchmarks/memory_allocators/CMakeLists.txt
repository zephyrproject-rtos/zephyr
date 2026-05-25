# Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(app)
target_sources(app PRIVATE
  src/k_heap.c
  src/mem_slab.c
  src/mempool.c
  src/sys_heap.c
  src/mem_blocks.c
)

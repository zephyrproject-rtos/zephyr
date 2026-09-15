# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(kernel_latency)

target_sources(app PRIVATE
  src/thread.c
  src/semaphore.c
  src/mutex.c
  src/fifo.c
  src/lifo.c
  src/stack.c
  src/heap.c
  src/condvar.c
)

target_sources_ifdef(CONFIG_EVENTS app PRIVATE src/events.c)

# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ringbuffer)

target_sources(app PRIVATE
    src/main.c
    src/concurrent.c
  )

target_sources_ifndef(CONFIG_RING_BUFFER_CLAIM
    app PRIVATE
    src/index_math.c
  )

target_sources_ifdef(CONFIG_RING_BUFFER_CLAIM
    app PRIVATE
    src/deprecated.c
  )

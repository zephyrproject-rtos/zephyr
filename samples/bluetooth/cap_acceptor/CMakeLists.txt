# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cap_acceptor)

target_sources(app PRIVATE
  src/main.c
)

zephyr_sources_ifdef(CONFIG_SAMPLE_UNICAST src/cap_acceptor_unicast.c)
zephyr_sources_ifdef(CONFIG_SAMPLE_BROADCAST src/cap_acceptor_broadcast.c)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

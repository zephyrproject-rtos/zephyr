# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(stun_client)

target_sources(app PRIVATE src/main.c src/server.c)
target_sources_ifdef(CONFIG_NET_SHELL app PRIVATE src/shell.c)

# What the client writes to the log is looked at where it can write to it, with
# the log backend of the tests of the codec.
if(CONFIG_NET_LOG AND NOT CONFIG_LOG_MODE_MINIMAL)
  target_sources(app PRIVATE src/log.c ../stun/src/log_capture.c)
  target_include_directories(app PRIVATE ../stun/src)
endif()

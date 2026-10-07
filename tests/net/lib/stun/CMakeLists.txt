# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-FileCopyrightText: Copyright (c) 2026 Kirill Shypachov
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(stun)

target_sources(app PRIVATE
  src/address.c
  src/binding.c
  src/builder.c
  src/example.c
  src/integrity.c
  src/main.c
  src/parse.c
)

# What the library writes to the log is looked at where it can write to it, and
# where the log has backends: the minimal one, which a test has by default, has
# none.
if(CONFIG_NET_LOG AND NOT CONFIG_LOG_MODE_MINIMAL)
  target_sources(app PRIVATE
    src/log.c
    src/log_capture.c
  )
endif()

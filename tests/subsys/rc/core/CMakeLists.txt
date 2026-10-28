# SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(rc_core)

target_sources(app PRIVATE
  src/event.c
  src/keystate.c
  src/main.c
  src/protocol.c
  src/raw.c
  src/rawio.c
  src/sync.c
)

if(CONFIG_TEST_RC_CALLBACKS)
  target_sources(app PRIVATE src/callback.c)
else()
  target_sources(app PRIVATE src/no_callback.c)
endif()

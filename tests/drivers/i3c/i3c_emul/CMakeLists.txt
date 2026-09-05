# Copyright (c) 2026 Meta Platforms
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(i3c_emul)

target_sources(app PRIVATE
  src/test_core.c
  src/test_ccc.c
  src/test_target_emul.c
  src/i2c_test_target_emul.c
)
target_sources_ifdef(CONFIG_I3C_USE_IBI app PRIVATE src/test_ibi.c)
target_sources_ifdef(CONFIG_I3C_CALLBACK app PRIVATE src/test_async.c)
target_sources_ifdef(CONFIG_I3C_TARGET app PRIVATE
  src/test_target.c
  src/test_controller_handoff.c
)
target_include_directories(app PRIVATE src)

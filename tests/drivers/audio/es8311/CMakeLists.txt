# Copyright (c) 2026 Hsiu-Chi Tsai
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(es8311_codec_test)

# The emulator's test-backend header lives beside the emulator (drivers/audio), which is
# where emul_es8311.c finds it same-directory; the test reaches it by putting that directory
# on its include path.
zephyr_include_directories(${ZEPHYR_BASE}/drivers/audio)

target_sources(app PRIVATE src/main.c)

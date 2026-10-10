# Copyright 2025 - 2026 NXP
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(amp_audio_output)
target_sources(app PRIVATE src/main.c)

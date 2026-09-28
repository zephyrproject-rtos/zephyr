# Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tracing_skip_internal_objects)

target_sources(app PRIVATE src/main.c)
# Observe every CTF event on its way into the tracing core
target_link_options(app PUBLIC -Wl,--wrap=tracing_format_raw_data)

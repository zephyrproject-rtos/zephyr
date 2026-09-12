# Copyright (c) 2026 EPAM Systems
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(arm64_fwargs)

target_link_options(app PUBLIC -Wl,--wrap=fwargs_handler_hook)
target_sources(app PRIVATE src/main.c)

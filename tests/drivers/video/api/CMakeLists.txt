# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(integration)

target_sources(app PRIVATE src/video_common.c)
target_sources(app PRIVATE src/video_emul.c)

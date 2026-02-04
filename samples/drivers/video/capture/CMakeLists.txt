# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(video_capture)

target_sources(app PRIVATE src/main.c)

if(CONFIG_VIDEO_TRANSFORM)
    target_sources(app PRIVATE src/transform_${CONFIG_VIDEO_TRANSFORM}.c)
endif()

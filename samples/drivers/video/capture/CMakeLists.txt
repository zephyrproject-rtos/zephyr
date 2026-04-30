# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(video_capture)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_VIDEO_TRANSFORM_NXP_JPEGDEC app PRIVATE src/transform_nxp_jpegdec.c)
target_sources_ifdef(CONFIG_VIDEO_TRANSFORM_NXP_PNGDEC app PRIVATE src/transform_nxp_pngdec.c)

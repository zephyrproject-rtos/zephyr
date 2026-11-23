# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(video_native_sim_fifo)

target_sources(app PRIVATE src/main.c)

# Built in the native simulator runner context, so that the test suite can feed
# the FIFO itself instead of depending on an external writer such as ffmpeg.
target_sources(native_simulator INTERFACE src/fifo_writer_bottom.c)

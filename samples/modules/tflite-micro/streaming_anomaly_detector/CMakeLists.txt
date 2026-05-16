# SPDX-FileCopyrightText: Copyright (c) 2026 Mahad Faisal
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tflm_streaming_anomaly_detector)

target_include_directories(app PRIVATE src)

target_sources(app PRIVATE
  src/assert.cpp
  src/input_stream.c
  src/main.c
  src/main_functions.cpp
  src/model.cpp
  src/ring_buffer.c
  src/stream_features.c
)

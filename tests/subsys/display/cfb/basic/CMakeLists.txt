# Copyright (c) 2024 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(cfb_basic)

target_sources(app PRIVATE
  ../common/utils.c
  src/testdata.c
  src/clear.c
  src/draw_point.c
  src/draw_line.c
  src/draw_rect.c
  src/draw_circle.c
  src/invert.c
  src/invert_area.c
  src/cfb_font_stub.c
)

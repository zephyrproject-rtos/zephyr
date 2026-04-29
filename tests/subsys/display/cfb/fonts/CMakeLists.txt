# Copyright (c) 2024 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(cfb_fonts)

if(CONFIG_TEST_MSB_FIRST_FONT)
  set(FONT_SRC src/cfb_font_rectspace1016_msb.c)
else()
  set(FONT_SRC src/cfb_font_rectspace1016.c)
endif()

target_sources(app PRIVATE
  ../common/utils.c
  src/testdata.c
  src/draw_text_rectspace1016.c
  src/print_rectspace1016.c
  ${FONT_SRC}
)

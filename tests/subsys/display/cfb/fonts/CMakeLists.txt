# Copyright (c) 2024 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(cfb_fonts)

if(NOT CONFIG_TEST_VPACKED_FONT)
  set(FONT_SRC
    src/cfb_font_rectspace1016_hpacked.c
    src/cfb_font_rectspace1123_hpacked.c
    src/cfb_font_dot0101_hpacked.c
  )
elseif(CONFIG_TEST_MSB_FIRST_FONT)
  set(FONT_SRC
    src/cfb_font_rectspace1016_msb.c
    src/cfb_font_rectspace1123_msb.c
    src/cfb_font_dot0101_msb.c
  )
else()
  set(FONT_SRC
    src/cfb_font_rectspace1016.c
    src/cfb_font_rectspace1123.c
    src/cfb_font_dot0101.c
  )
endif()

target_sources(app PRIVATE
  ../common/utils.c
  src/testdata.c
  src/draw_text_rectspace1016.c
  src/draw_text_rectspace1123.c
  src/draw_text_dot0101.c
  src/print_rectspace1016.c
  src/print_rectspace1123.c
  src/print_dot0101.c
  ${FONT_SRC}
)

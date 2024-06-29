# Copyright (c) 2024 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(cfb_basic)

if(CONFIG_TEST_MSB_FIRST_FONT)
	target_sources(app PRIVATE src/testdata.c
				   src/utils.c
				   src/clear.c
				   src/draw_point.c
				   src/draw_line.c
				   src/draw_rect.c
				   src/draw_text_rectspace1016.c
				   src/invert.c
				   src/invert_area.c
				   src/print_rectspace1016.c
				   src/cfb_font_rectspace1016_msb.c)
else()
	target_sources(app PRIVATE src/testdata.c
				   src/utils.c
				   src/clear.c
				   src/draw_point.c
				   src/draw_line.c
				   src/draw_rect.c
				   src/draw_text_rectspace1016.c
				   src/invert.c
				   src/invert_area.c
				   src/print_rectspace1016.c
				   src/cfb_font_rectspace1016.c)
endif()

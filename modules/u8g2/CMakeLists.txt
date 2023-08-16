# Copyright (c) 2023 TOKITA Hiroshi
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_U8G2)
	set(U8G2_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

	zephyr_library()

	zephyr_include_directories(
		${U8G2_DIR}/csrc
		${CMAKE_CURRENT_SOURCE_DIR}
	)

	zephyr_library_sources(
		${U8G2_DIR}/csrc/mui.c
		${U8G2_DIR}/csrc/mui_u8g2.c
		${U8G2_DIR}/csrc/u8g2_bitmap.c
		${U8G2_DIR}/csrc/u8g2_box.c
		${U8G2_DIR}/csrc/u8g2_buffer.c
		${U8G2_DIR}/csrc/u8g2_button.c
		${U8G2_DIR}/csrc/u8g2_circle.c
		${U8G2_DIR}/csrc/u8g2_cleardisplay.c
		${U8G2_DIR}/csrc/u8g2_d_memory.c
		${U8G2_DIR}/csrc/u8g2_d_setup.c
		${U8G2_DIR}/csrc/u8g2_font.c
		${U8G2_DIR}/csrc/u8g2_fonts.c
		${U8G2_DIR}/csrc/u8g2_hvline.c
		${U8G2_DIR}/csrc/u8g2_input_value.c
		${U8G2_DIR}/csrc/u8g2_intersection.c
		${U8G2_DIR}/csrc/u8g2_kerning.c
		${U8G2_DIR}/csrc/u8g2_line.c
		${U8G2_DIR}/csrc/u8g2_ll_hvline.c
		${U8G2_DIR}/csrc/u8g2_message.c
		${U8G2_DIR}/csrc/u8g2_polygon.c
		${U8G2_DIR}/csrc/u8g2_selection_list.c
		${U8G2_DIR}/csrc/u8g2_setup.c
		${U8G2_DIR}/csrc/u8log.c
		${U8G2_DIR}/csrc/u8log_u8g2.c
		${U8G2_DIR}/csrc/u8log_u8x8.c
		${U8G2_DIR}/csrc/u8x8_8x8.c
		${U8G2_DIR}/csrc/u8x8_byte.c
		${U8G2_DIR}/csrc/u8x8_cad.c
		${U8G2_DIR}/csrc/u8x8_capture.c
		${U8G2_DIR}/csrc/u8x8_debounce.c
		${U8G2_DIR}/csrc/u8x8_display.c
		${U8G2_DIR}/csrc/u8x8_fonts.c
		${U8G2_DIR}/csrc/u8x8_gpio.c
		${U8G2_DIR}/csrc/u8x8_input_value.c
		${U8G2_DIR}/csrc/u8x8_message.c
		${U8G2_DIR}/csrc/u8x8_selection_list.c
		${U8G2_DIR}/csrc/u8x8_setup.c
		${U8G2_DIR}/csrc/u8x8_string.c
		${U8G2_DIR}/csrc/u8x8_u16toa.c
		${U8G2_DIR}/csrc/u8x8_u8toa.c
		${CMAKE_CURRENT_SOURCE_DIR}/u8x8_d_zephyr.c
	)
endif()

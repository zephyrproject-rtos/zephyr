/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_SUBSYS_DISPLAY_CFB_DRAW_TEXT_AND_PRINT_SRC_UTILS_H__
#define TESTS_SUBSYS_DISPLAY_CFB_DRAW_TEXT_AND_PRINT_SRC_UTILS_H__

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/devicetree.h>

extern uint8_t read_buffer[DT_PROP(DT_CHOSEN(zephyr_display), width) *
			   DT_PROP(DT_CHOSEN(zephyr_display), height) * 4];
extern uint32_t transfer_buf[CONFIG_TEST_CFB_TRANSFER_BUF_SIZE];
extern uint32_t command_buf[CONFIG_TEST_CFB_COMMAND_BUF_SIZE];

uint32_t display_pixel(int x, int y);
uint32_t image_pixel(const uint32_t *img, size_t width, int x, int y);
bool verify_pixel(int x, int y, uint32_t color);
bool verify_image(int x, int y, const uint32_t *img, size_t width, size_t height);
bool verify_color_inside_rect(int x, int y, size_t width, size_t height, uint32_t color);
bool verify_color_outside_rect(int x, int y, size_t width, size_t height, uint32_t color);
bool verify_image_and_bg(int x, int y, const uint32_t *img, size_t width, size_t height,
			 uint32_t color);
bool verify_pixel_and_bg(int x, int y, uint32_t pixcolor, uint32_t bgcolor);

#endif /* TESTS_SUBSYS_DISPLAY_CFB_DRAW_TEXT_AND_PRINT_SRC_UTILS_H__ */

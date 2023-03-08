/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

/* 0-255 based on color codes */
struct rgb_color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

const struct rgb_color none = {
	.red = 0,
	.green = 0,
	.blue = 0
};

const struct rgb_color red = {
	.red = 255,
	.green = 0,
	.blue = 0,
};

const struct rgb_color lime = {
	.red = 0,
	.green = 255,
	.blue = 0
};

const struct rgb_color yellow = {
	.red = 255,
	.green = 255,
	.blue = 0
};

const struct rgb_color orange = {
	.red = 255,
	.green = 165,
	.blue = 0
};

const struct rgb_color green = {
	.red = 0,
	.green = 128,
	.blue = 0,
};

const struct rgb_color cyan = {
	.red = 0,
	.green = 255,
	.blue = 255
};

const struct rgb_color aquamarine = {
	.red = 102,
	.green = 205,
	.blue = 170
};

const struct rgb_color blue = {
	.red = 0,
	.green = 0,
	.blue = 255
};

const struct rgb_color purple = {
	.red = 128,
	.green = 0,
	.blue = 128
};

const struct rgb_color pink = {
	.red = 255,
	.green = 192,
	.blue = 203
};

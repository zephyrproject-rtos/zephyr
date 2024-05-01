/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_DECLARE(net_http_server, CONFIG_NET_HTTP_SERVER_LOG_LEVEL);

struct decode_elem {
	uint8_t bitlen;
	uint8_t symbol;
	uint8_t code[4];
};

static const struct decode_elem decode_table[] = {
	{  5,  48, { 0b00000000, 0b00000000, 0b00000000, 0b00000000 } },
	{  5,  49, { 0b00001000, 0b00000000, 0b00000000, 0b00000000 } },
	{  5,  50, { 0b00010000, 0b00000000, 0b00000000, 0b00000000 } },
	{  5,  97, { 0b00011000, 0b00000000, 0b00000000, 0b00000000 } },
	{  5,  99, { 0b00100000, 0b00000000, 0b00000000, 0b00000000 } },
	{  5, 101, { 0b00101000, 0b00000000, 0b00000000, 0b00000000 } },
	{  5, 105, { 0b00110000, 0b00000000, 0b00000000, 0b00000000 } },
	{  5, 111, { 0b00111000, 0b00000000, 0b00000000, 0b00000000 } },
	{  5, 115, { 0b01000000, 0b00000000, 0b00000000, 0b00000000 } },
	{  5, 116, { 0b01001000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  32, { 0b01010000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  37, { 0b01010100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  45, { 0b01011000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  46, { 0b01011100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  47, { 0b01100000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  51, { 0b01100100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  52, { 0b01101000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  53, { 0b01101100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  54, { 0b01110000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  55, { 0b01110100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  56, { 0b01111000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  57, { 0b01111100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  61, { 0b10000000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  65, { 0b10000100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  95, { 0b10001000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6,  98, { 0b10001100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 100, { 0b10010000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 102, { 0b10010100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 103, { 0b10011000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 104, { 0b10011100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 108, { 0b10100000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 109, { 0b10100100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 110, { 0b10101000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 112, { 0b10101100, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 114, { 0b10110000, 0b00000000, 0b00000000, 0b00000000 } },
	{  6, 117, { 0b10110100, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  58, { 0b10111000, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  66, { 0b10111010, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  67, { 0b10111100, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  68, { 0b10111110, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  69, { 0b11000000, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  70, { 0b11000010, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  71, { 0b11000100, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  72, { 0b11000110, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  73, { 0b11001000, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  74, { 0b11001010, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  75, { 0b11001100, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  76, { 0b11001110, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  77, { 0b11010000, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  78, { 0b11010010, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  79, { 0b11010100, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  80, { 0b11010110, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  81, { 0b11011000, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  82, { 0b11011010, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  83, { 0b11011100, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  84, { 0b11011110, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  85, { 0b11100000, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  86, { 0b11100010, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  87, { 0b11100100, 0b00000000, 0b00000000, 0b00000000 } },
	{  7,  89, { 0b11100110, 0b00000000, 0b00000000, 0b00000000 } },
	{  7, 106, { 0b11101000, 0b00000000, 0b00000000, 0b00000000 } },
	{  7, 107, { 0b11101010, 0b00000000, 0b00000000, 0b00000000 } },
	{  7, 113, { 0b11101100, 0b00000000, 0b00000000, 0b00000000 } },
	{  7, 118, { 0b11101110, 0b00000000, 0b00000000, 0b00000000 } },
	{  7, 119, { 0b11110000, 0b00000000, 0b00000000, 0b00000000 } },
	{  7, 120, { 0b11110010, 0b00000000, 0b00000000, 0b00000000 } },
	{  7, 121, { 0b11110100, 0b00000000, 0b00000000, 0b00000000 } },
	{  7, 122, { 0b11110110, 0b00000000, 0b00000000, 0b00000000 } },
	{  8,  38, { 0b11111000, 0b00000000, 0b00000000, 0b00000000 } },
	{  8,  42, { 0b11111001, 0b00000000, 0b00000000, 0b00000000 } },
	{  8,  44, { 0b11111010, 0b00000000, 0b00000000, 0b00000000 } },
	{  8,  59, { 0b11111011, 0b00000000, 0b00000000, 0b00000000 } },
	{  8,  88, { 0b11111100, 0b00000000, 0b00000000, 0b00000000 } },
	{  8,  90, { 0b11111101, 0b00000000, 0b00000000, 0b00000000 } },
	{ 10,  33, { 0b11111110, 0b00000000, 0b00000000, 0b00000000 } },
	{ 10,  34, { 0b11111110, 0b01000000, 0b00000000, 0b00000000 } },
	{ 10,  40, { 0b11111110, 0b10000000, 0b00000000, 0b00000000 } },
	{ 10,  41, { 0b11111110, 0b11000000, 0b00000000, 0b00000000 } },
	{ 10,  63, { 0b11111111, 0b00000000, 0b00000000, 0b00000000 } },
	{ 11,  39, { 0b11111111, 0b01000000, 0b00000000, 0b00000000 } },
	{ 11,  43, { 0b11111111, 0b01100000, 0b00000000, 0b00000000 } },
	{ 11, 124, { 0b11111111, 0b10000000, 0b00000000, 0b00000000 } },
	{ 12,  35, { 0b11111111, 0b10100000, 0b00000000, 0b00000000 } },
	{ 12,  62, { 0b11111111, 0b10110000, 0b00000000, 0b00000000 } },
	{ 13,   0, { 0b11111111, 0b11000000, 0b00000000, 0b00000000 } },
	{ 13,  36, { 0b11111111, 0b11001000, 0b00000000, 0b00000000 } },
	{ 13,  64, { 0b11111111, 0b11010000, 0b00000000, 0b00000000 } },
	{ 13,  91, { 0b11111111, 0b11011000, 0b00000000, 0b00000000 } },
	{ 13,  93, { 0b11111111, 0b11100000, 0b00000000, 0b00000000 } },
	{ 13, 126, { 0b11111111, 0b11101000, 0b00000000, 0b00000000 } },
	{ 14,  94, { 0b11111111, 0b11110000, 0b00000000, 0b00000000 } },
	{ 14, 125, { 0b11111111, 0b11110100, 0b00000000, 0b00000000 } },
	{ 15,  60, { 0b11111111, 0b11111000, 0b00000000, 0b00000000 } },
	{ 15,  96, { 0b11111111, 0b11111010, 0b00000000, 0b00000000 } },
	{ 15, 123, { 0b11111111, 0b11111100, 0b00000000, 0b00000000 } },
	{ 19,  92, { 0b11111111, 0b11111110, 0b00000000, 0b00000000 } },
	{ 19, 195, { 0b11111111, 0b11111110, 0b00100000, 0b00000000 } },
	{ 19, 208, { 0b11111111, 0b11111110, 0b01000000, 0b00000000 } },
	{ 20, 128, { 0b11111111, 0b11111110, 0b01100000, 0b00000000 } },
	{ 20, 130, { 0b11111111, 0b11111110, 0b01110000, 0b00000000 } },
	{ 20, 131, { 0b11111111, 0b11111110, 0b10000000, 0b00000000 } },
	{ 20, 162, { 0b11111111, 0b11111110, 0b10010000, 0b00000000 } },
	{ 20, 184, { 0b11111111, 0b11111110, 0b10100000, 0b00000000 } },
	{ 20, 194, { 0b11111111, 0b11111110, 0b10110000, 0b00000000 } },
	{ 20, 224, { 0b11111111, 0b11111110, 0b11000000, 0b00000000 } },
	{ 20, 226, { 0b11111111, 0b11111110, 0b11010000, 0b00000000 } },
	{ 21, 153, { 0b11111111, 0b11111110, 0b11100000, 0b00000000 } },
	{ 21, 161, { 0b11111111, 0b11111110, 0b11101000, 0b00000000 } },
	{ 21, 167, { 0b11111111, 0b11111110, 0b11110000, 0b00000000 } },
	{ 21, 172, { 0b11111111, 0b11111110, 0b11111000, 0b00000000 } },
	{ 21, 176, { 0b11111111, 0b11111111, 0b00000000, 0b00000000 } },
	{ 21, 177, { 0b11111111, 0b11111111, 0b00001000, 0b00000000 } },
	{ 21, 179, { 0b11111111, 0b11111111, 0b00010000, 0b00000000 } },
	{ 21, 209, { 0b11111111, 0b11111111, 0b00011000, 0b00000000 } },
	{ 21, 216, { 0b11111111, 0b11111111, 0b00100000, 0b00000000 } },
	{ 21, 217, { 0b11111111, 0b11111111, 0b00101000, 0b00000000 } },
	{ 21, 227, { 0b11111111, 0b11111111, 0b00110000, 0b00000000 } },
	{ 21, 229, { 0b11111111, 0b11111111, 0b00111000, 0b00000000 } },
	{ 21, 230, { 0b11111111, 0b11111111, 0b01000000, 0b00000000 } },
	{ 22, 129, { 0b11111111, 0b11111111, 0b01001000, 0b00000000 } },
	{ 22, 132, { 0b11111111, 0b11111111, 0b01001100, 0b00000000 } },
	{ 22, 133, { 0b11111111, 0b11111111, 0b01010000, 0b00000000 } },
	{ 22, 134, { 0b11111111, 0b11111111, 0b01010100, 0b00000000 } },
	{ 22, 136, { 0b11111111, 0b11111111, 0b01011000, 0b00000000 } },
	{ 22, 146, { 0b11111111, 0b11111111, 0b01011100, 0b00000000 } },
	{ 22, 154, { 0b11111111, 0b11111111, 0b01100000, 0b00000000 } },
	{ 22, 156, { 0b11111111, 0b11111111, 0b01100100, 0b00000000 } },
	{ 22, 160, { 0b11111111, 0b11111111, 0b01101000, 0b00000000 } },
	{ 22, 163, { 0b11111111, 0b11111111, 0b01101100, 0b00000000 } },
	{ 22, 164, { 0b11111111, 0b11111111, 0b01110000, 0b00000000 } },
	{ 22, 169, { 0b11111111, 0b11111111, 0b01110100, 0b00000000 } },
	{ 22, 170, { 0b11111111, 0b11111111, 0b01111000, 0b00000000 } },
	{ 22, 173, { 0b11111111, 0b11111111, 0b01111100, 0b00000000 } },
	{ 22, 178, { 0b11111111, 0b11111111, 0b10000000, 0b00000000 } },
	{ 22, 181, { 0b11111111, 0b11111111, 0b10000100, 0b00000000 } },
	{ 22, 185, { 0b11111111, 0b11111111, 0b10001000, 0b00000000 } },
	{ 22, 186, { 0b11111111, 0b11111111, 0b10001100, 0b00000000 } },
	{ 22, 187, { 0b11111111, 0b11111111, 0b10010000, 0b00000000 } },
	{ 22, 189, { 0b11111111, 0b11111111, 0b10010100, 0b00000000 } },
	{ 22, 190, { 0b11111111, 0b11111111, 0b10011000, 0b00000000 } },
	{ 22, 196, { 0b11111111, 0b11111111, 0b10011100, 0b00000000 } },
	{ 22, 198, { 0b11111111, 0b11111111, 0b10100000, 0b00000000 } },
	{ 22, 228, { 0b11111111, 0b11111111, 0b10100100, 0b00000000 } },
	{ 22, 232, { 0b11111111, 0b11111111, 0b10101000, 0b00000000 } },
	{ 22, 233, { 0b11111111, 0b11111111, 0b10101100, 0b00000000 } },
	{ 23,   1, { 0b11111111, 0b11111111, 0b10110000, 0b00000000 } },
	{ 23, 135, { 0b11111111, 0b11111111, 0b10110010, 0b00000000 } },
	{ 23, 137, { 0b11111111, 0b11111111, 0b10110100, 0b00000000 } },
	{ 23, 138, { 0b11111111, 0b11111111, 0b10110110, 0b00000000 } },
	{ 23, 139, { 0b11111111, 0b11111111, 0b10111000, 0b00000000 } },
	{ 23, 140, { 0b11111111, 0b11111111, 0b10111010, 0b00000000 } },
	{ 23, 141, { 0b11111111, 0b11111111, 0b10111100, 0b00000000 } },
	{ 23, 143, { 0b11111111, 0b11111111, 0b10111110, 0b00000000 } },
	{ 23, 147, { 0b11111111, 0b11111111, 0b11000000, 0b00000000 } },
	{ 23, 149, { 0b11111111, 0b11111111, 0b11000010, 0b00000000 } },
	{ 23, 150, { 0b11111111, 0b11111111, 0b11000100, 0b00000000 } },
	{ 23, 151, { 0b11111111, 0b11111111, 0b11000110, 0b00000000 } },
	{ 23, 152, { 0b11111111, 0b11111111, 0b11001000, 0b00000000 } },
	{ 23, 155, { 0b11111111, 0b11111111, 0b11001010, 0b00000000 } },
	{ 23, 157, { 0b11111111, 0b11111111, 0b11001100, 0b00000000 } },
	{ 23, 158, { 0b11111111, 0b11111111, 0b11001110, 0b00000000 } },
	{ 23, 165, { 0b11111111, 0b11111111, 0b11010000, 0b00000000 } },
	{ 23, 166, { 0b11111111, 0b11111111, 0b11010010, 0b00000000 } },
	{ 23, 168, { 0b11111111, 0b11111111, 0b11010100, 0b00000000 } },
	{ 23, 174, { 0b11111111, 0b11111111, 0b11010110, 0b00000000 } },
	{ 23, 175, { 0b11111111, 0b11111111, 0b11011000, 0b00000000 } },
	{ 23, 180, { 0b11111111, 0b11111111, 0b11011010, 0b00000000 } },
	{ 23, 182, { 0b11111111, 0b11111111, 0b11011100, 0b00000000 } },
	{ 23, 183, { 0b11111111, 0b11111111, 0b11011110, 0b00000000 } },
	{ 23, 188, { 0b11111111, 0b11111111, 0b11100000, 0b00000000 } },
	{ 23, 191, { 0b11111111, 0b11111111, 0b11100010, 0b00000000 } },
	{ 23, 197, { 0b11111111, 0b11111111, 0b11100100, 0b00000000 } },
	{ 23, 231, { 0b11111111, 0b11111111, 0b11100110, 0b00000000 } },
	{ 23, 239, { 0b11111111, 0b11111111, 0b11101000, 0b00000000 } },
	{ 24,   9, { 0b11111111, 0b11111111, 0b11101010, 0b00000000 } },
	{ 24, 142, { 0b11111111, 0b11111111, 0b11101011, 0b00000000 } },
	{ 24, 144, { 0b11111111, 0b11111111, 0b11101100, 0b00000000 } },
	{ 24, 145, { 0b11111111, 0b11111111, 0b11101101, 0b00000000 } },
	{ 24, 148, { 0b11111111, 0b11111111, 0b11101110, 0b00000000 } },
	{ 24, 159, { 0b11111111, 0b11111111, 0b11101111, 0b00000000 } },
	{ 24, 171, { 0b11111111, 0b11111111, 0b11110000, 0b00000000 } },
	{ 24, 206, { 0b11111111, 0b11111111, 0b11110001, 0b00000000 } },
	{ 24, 215, { 0b11111111, 0b11111111, 0b11110010, 0b00000000 } },
	{ 24, 225, { 0b11111111, 0b11111111, 0b11110011, 0b00000000 } },
	{ 24, 236, { 0b11111111, 0b11111111, 0b11110100, 0b00000000 } },
	{ 24, 237, { 0b11111111, 0b11111111, 0b11110101, 0b00000000 } },
	{ 25, 199, { 0b11111111, 0b11111111, 0b11110110, 0b00000000 } },
	{ 25, 207, { 0b11111111, 0b11111111, 0b11110110, 0b10000000 } },
	{ 25, 234, { 0b11111111, 0b11111111, 0b11110111, 0b00000000 } },
	{ 25, 235, { 0b11111111, 0b11111111, 0b11110111, 0b10000000 } },
	{ 26, 192, { 0b11111111, 0b11111111, 0b11111000, 0b00000000 } },
	{ 26, 193, { 0b11111111, 0b11111111, 0b11111000, 0b01000000 } },
	{ 26, 200, { 0b11111111, 0b11111111, 0b11111000, 0b10000000 } },
	{ 26, 201, { 0b11111111, 0b11111111, 0b11111000, 0b11000000 } },
	{ 26, 202, { 0b11111111, 0b11111111, 0b11111001, 0b00000000 } },
	{ 26, 205, { 0b11111111, 0b11111111, 0b11111001, 0b01000000 } },
	{ 26, 210, { 0b11111111, 0b11111111, 0b11111001, 0b10000000 } },
	{ 26, 213, { 0b11111111, 0b11111111, 0b11111001, 0b11000000 } },
	{ 26, 218, { 0b11111111, 0b11111111, 0b11111010, 0b00000000 } },
	{ 26, 219, { 0b11111111, 0b11111111, 0b11111010, 0b01000000 } },
	{ 26, 238, { 0b11111111, 0b11111111, 0b11111010, 0b10000000 } },
	{ 26, 240, { 0b11111111, 0b11111111, 0b11111010, 0b11000000 } },
	{ 26, 242, { 0b11111111, 0b11111111, 0b11111011, 0b00000000 } },
	{ 26, 243, { 0b11111111, 0b11111111, 0b11111011, 0b01000000 } },
	{ 26, 255, { 0b11111111, 0b11111111, 0b11111011, 0b10000000 } },
	{ 27, 203, { 0b11111111, 0b11111111, 0b11111011, 0b11000000 } },
	{ 27, 204, { 0b11111111, 0b11111111, 0b11111011, 0b11100000 } },
	{ 27, 211, { 0b11111111, 0b11111111, 0b11111100, 0b00000000 } },
	{ 27, 212, { 0b11111111, 0b11111111, 0b11111100, 0b00100000 } },
	{ 27, 214, { 0b11111111, 0b11111111, 0b11111100, 0b01000000 } },
	{ 27, 221, { 0b11111111, 0b11111111, 0b11111100, 0b01100000 } },
	{ 27, 222, { 0b11111111, 0b11111111, 0b11111100, 0b10000000 } },
	{ 27, 223, { 0b11111111, 0b11111111, 0b11111100, 0b10100000 } },
	{ 27, 241, { 0b11111111, 0b11111111, 0b11111100, 0b11000000 } },
	{ 27, 244, { 0b11111111, 0b11111111, 0b11111100, 0b11100000 } },
	{ 27, 245, { 0b11111111, 0b11111111, 0b11111101, 0b00000000 } },
	{ 27, 246, { 0b11111111, 0b11111111, 0b11111101, 0b00100000 } },
	{ 27, 247, { 0b11111111, 0b11111111, 0b11111101, 0b01000000 } },
	{ 27, 248, { 0b11111111, 0b11111111, 0b11111101, 0b01100000 } },
	{ 27, 250, { 0b11111111, 0b11111111, 0b11111101, 0b10000000 } },
	{ 27, 251, { 0b11111111, 0b11111111, 0b11111101, 0b10100000 } },
	{ 27, 252, { 0b11111111, 0b11111111, 0b11111101, 0b11000000 } },
	{ 27, 253, { 0b11111111, 0b11111111, 0b11111101, 0b11100000 } },
	{ 27, 254, { 0b11111111, 0b11111111, 0b11111110, 0b00000000 } },
	{ 28,   2, { 0b11111111, 0b11111111, 0b11111110, 0b00100000 } },
	{ 28,   3, { 0b11111111, 0b11111111, 0b11111110, 0b00110000 } },
	{ 28,   4, { 0b11111111, 0b11111111, 0b11111110, 0b01000000 } },
	{ 28,   5, { 0b11111111, 0b11111111, 0b11111110, 0b01010000 } },
	{ 28,   6, { 0b11111111, 0b11111111, 0b11111110, 0b01100000 } },
	{ 28,   7, { 0b11111111, 0b11111111, 0b11111110, 0b01110000 } },
	{ 28,   8, { 0b11111111, 0b11111111, 0b11111110, 0b10000000 } },
	{ 28,  11, { 0b11111111, 0b11111111, 0b11111110, 0b10010000 } },
	{ 28,  12, { 0b11111111, 0b11111111, 0b11111110, 0b10100000 } },
	{ 28,  14, { 0b11111111, 0b11111111, 0b11111110, 0b10110000 } },
	{ 28,  15, { 0b11111111, 0b11111111, 0b11111110, 0b11000000 } },
	{ 28,  16, { 0b11111111, 0b11111111, 0b11111110, 0b11010000 } },
	{ 28,  17, { 0b11111111, 0b11111111, 0b11111110, 0b11100000 } },
	{ 28,  18, { 0b11111111, 0b11111111, 0b11111110, 0b11110000 } },
	{ 28,  19, { 0b11111111, 0b11111111, 0b11111111, 0b00000000 } },
	{ 28,  20, { 0b11111111, 0b11111111, 0b11111111, 0b00010000 } },
	{ 28,  21, { 0b11111111, 0b11111111, 0b11111111, 0b00100000 } },
	{ 28,  23, { 0b11111111, 0b11111111, 0b11111111, 0b00110000 } },
	{ 28,  24, { 0b11111111, 0b11111111, 0b11111111, 0b01000000 } },
	{ 28,  25, { 0b11111111, 0b11111111, 0b11111111, 0b01010000 } },
	{ 28,  26, { 0b11111111, 0b11111111, 0b11111111, 0b01100000 } },
	{ 28,  27, { 0b11111111, 0b11111111, 0b11111111, 0b01110000 } },
	{ 28,  28, { 0b11111111, 0b11111111, 0b11111111, 0b10000000 } },
	{ 28,  29, { 0b11111111, 0b11111111, 0b11111111, 0b10010000 } },
	{ 28,  30, { 0b11111111, 0b11111111, 0b11111111, 0b10100000 } },
	{ 28,  31, { 0b11111111, 0b11111111, 0b11111111, 0b10110000 } },
	{ 28, 127, { 0b11111111, 0b11111111, 0b11111111, 0b11000000 } },
	{ 28, 220, { 0b11111111, 0b11111111, 0b11111111, 0b11010000 } },
	{ 28, 249, { 0b11111111, 0b11111111, 0b11111111, 0b11100000 } },
	{ 30,  10, { 0b11111111, 0b11111111, 0b11111111, 0b11110000 } },
	{ 30,  13, { 0b11111111, 0b11111111, 0b11111111, 0b11110100 } },
	{ 30,  22, { 0b11111111, 0b11111111, 0b11111111, 0b11111000 } },
};

static const struct decode_elem eos = {
	30,   0, { 0b11111111, 0b11111111, 0b11111111, 0b11111100 }
};

#define UINT32_BITLEN 32

#define MSB_MASK(len) (UINT32_MAX << (UINT32_BITLEN - len))
#define LSB_MASK(len) ((1UL << len) - 1UL)

static bool huffman_bits_compare(uint32_t bits, const struct decode_elem *entry)
{
	uint32_t mask = MSB_MASK(entry->bitlen);
	uint32_t code = sys_get_be32(entry->code);

	if (code == (bits & mask)) {
		return true;
	}

	return false;
}

static const struct decode_elem *huffman_decode_bits(uint32_t bits)
{
	for (int i = 0; i < ARRAY_SIZE(decode_table); i++) {
		if (huffman_bits_compare(bits, &decode_table[i])) {
			return &decode_table[i];
		}
	}

	if (huffman_bits_compare(bits, &eos)) {
		return &eos;
	}

	return NULL;
}

static const struct decode_elem *huffman_find_entry(uint8_t symbol)
{
	for (int i = 0; i < ARRAY_SIZE(decode_table); i++) {
		if (decode_table[i].symbol == symbol) {
			return &decode_table[i];
		}
	}

	return NULL;
}

#define MAX_PADDING_LEN 7

int http_hpack_huffman_decode(const uint8_t *encoded_buf, size_t encoded_len,
			      uint8_t *buf, size_t buflen)
{
	size_t encoded_bits_len = encoded_len * 8;
	uint8_t bits_needed = UINT32_BITLEN;
	const struct decode_elem *decoded;
	uint8_t bits_in_byte_left = 8;
	size_t decoded_len = 0;
	uint32_t bits = 0;

	if (encoded_buf == NULL || buf == NULL || encoded_len == 0) {
		return -EINVAL;
	}

	while (encoded_bits_len > 0) {
		/* Refill the bits variable */
		while (bits_needed > 0) {
			if (encoded_len > 0) {
				if (bits_in_byte_left <= bits_needed) {
					/* Consume rest of the byte */
					bits <<= bits_in_byte_left;
					bits |= *encoded_buf &
						    LSB_MASK(bits_in_byte_left);
					bits_needed -= bits_in_byte_left;
					bits_in_byte_left = 0;
				} else {
					/* Consume part of the byte */
					bits <<= bits_needed;
					bits |= (*encoded_buf >>
						 (bits_in_byte_left - bits_needed)) &
						LSB_MASK(bits_needed);
					bits_in_byte_left -= bits_needed;
					bits_needed = 0;
				}
			} else {
				/* Pad with ones */
				bits <<= bits_needed;
				bits |= LSB_MASK(bits_needed);
				bits_needed = 0;
			}

			/* Move to next encoded byte */
			if (bits_in_byte_left == 0) {
				encoded_buf++;
				encoded_len--;
				bits_in_byte_left = 8;
			}
		}

		/* Pass to decoder */
		decoded = huffman_decode_bits(bits);
		if (decoded == NULL) {
			LOG_ERR("No symbol found");
			return -EBADMSG;
		}

		if (decoded == &eos) {
			if (encoded_bits_len > MAX_PADDING_LEN) {
				LOG_ERR("eos reached prematurely");
				return -EBADMSG;
			}

			break;
		}

		if (encoded_bits_len < decoded->bitlen) {
			LOG_ERR("Invalid symbol used for padding");
			return -EBADMSG;
		}

		/* Remove consumed bits from bits variable. */
		bits_needed += decoded->bitlen;
		encoded_bits_len -= decoded->bitlen;

		/* Store decoded symbol */
		if (buflen == 0) {
			LOG_ERR("Not enough buffer to decode string");
			return -ENOBUFS;
		}

		*buf = decoded->symbol;
		buf++;
		buflen--;
		decoded_len++;
	}

	return decoded_len;
}

int http_hpack_huffman_encode(const uint8_t *str, size_t str_len,
			      uint8_t *buf, size_t buflen)
{
	const struct decode_elem *entry;
	size_t buflen_bits = buflen * 8;
	uint8_t bit_offset = 0;
	int len = 0;

	if (str == NULL || buf == NULL || str_len == 0) {
		return -EINVAL;
	}

	while (str_len > 0) {
		uint32_t code;
		uint8_t bitlen;

		entry = huffman_find_entry(*str);
		if (entry == NULL) {
			return -EINVAL;
		}

		if (entry->bitlen > buflen_bits) {
			return -ENOBUFS;
		}

		bitlen = entry->bitlen;
		code = sys_get_be32(entry->code);

		while (bitlen > 0) {
			uint8_t to_copy = MIN(8 - bit_offset, bitlen);
			uint8_t byte = (uint8_t)((code & MSB_MASK(to_copy)) >>
						 (24 + bit_offset));

			/* This is way suboptimal */
			if (bit_offset == 0) {
				*buf = byte;
			} else {
				*buf |= byte;
			}

			code <<= to_copy;
			bitlen -= to_copy;
			bit_offset  = (bit_offset + to_copy) % 8;

			if (bit_offset == 0) {
				buf++;
				len++;
			}
		}

		buflen_bits -= entry->bitlen;
		str_len--;
		str++;
	}

	/* Pad with ones. */
	if (bit_offset > 0) {
		*buf |= LSB_MASK((8 - bit_offset));
		len++;
	}

	return len;
}

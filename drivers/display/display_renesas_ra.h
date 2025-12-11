/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_RENESAS_RA_H_
#define ZEPHYR_DRIVERS_DISPLAY_RENESAS_RA_H_

#include <zephyr/drivers/display.h>

#define ROUND_UP_64BYTES(x)    ROUND_UP(x, NUM_BITS(uint64_t))
#define INPUT_FORMAT_PIXEL(n)  DT_INST_PROP(n, input_pixel_format)
#define OUTPUT_FORMAT_PIXEL(n) DT_INST_PROP(n, output_pixel_format)

#define RENESAS_RA_GLCDC_IN_PIXEL_FORMAT_1  (DISPLAY_IN_FORMAT_32BITS_RGB888)
#define RENESAS_RA_GLCDC_IN_PIXEL_FORMAT_8  (DISPLAY_IN_FORMAT_32BITS_ARGB8888)
#define RENESAS_RA_GLCDC_IN_PIXEL_FORMAT_16 (DISPLAY_IN_FORMAT_16BITS_RGB565)

#define RENESAS_RA_GLCDC_OUT_PIXEL_FORMAT_1  (DISPLAY_OUT_FORMAT_24BITS_RGB888)
#define RENESAS_RA_GLCDC_OUT_PIXEL_FORMAT_16 (DISPLAY_OUT_FORMAT_16BITS_RGB565)

#define RENESAS_RA_DISPLAY_GET_PIXEL_FORMAT(n)                                                     \
	(DT_INST_PROP(n, input_pixel_format) == PANEL_PIXEL_FORMAT_RGB_888                         \
		 ? PANEL_PIXEL_FORMAT_ARGB_8888                                                    \
		 : DT_INST_PROP(n, input_pixel_format))

#define DISPLAY_HSIZE(n) (DT_INST_PROP(n, width))
#define DISPLAY_VSIZE(n) (DT_INST_PROP(n, height))

#define RENESAS_RA_GLCDC_IN_PIXEL_FORMAT(n)                                                        \
	UTIL_CAT(RENESAS_RA_GLCDC_IN_PIXEL_FORMAT_, INPUT_FORMAT_PIXEL(n))

#define RENESAS_RA_GLCDC_OUT_PIXEL_FORMAT(n)                                                       \
	UTIL_CAT(RENESAS_RA_GLCDC_OUT_PIXEL_FORMAT_, OUTPUT_FORMAT_PIXEL(n))

#define RENESAS_RA_GLCDC_PIXEL_BYTE_SIZE(n)                                                        \
	(DISPLAY_BITS_PER_PIXEL(RENESAS_RA_DISPLAY_GET_PIXEL_FORMAT(n)) >> 3)

#define RENESAS_RA_DISPLAY_BUFFER_HSTRIDE_BYTE(n)                                                  \
	(ROUND_UP_64BYTES(DISPLAY_HSIZE(n) * DISPLAY_BITS_PER_PIXEL(INPUT_FORMAT_PIXEL(n))) /      \
	 DISPLAY_BITS_PER_PIXEL(INPUT_FORMAT_PIXEL(n)))

#define RENESAS_RA_GLCDC_HTIMING(n)                                                                \
	{.total_cyc = DT_INST_PROP(n, width) +                                                     \
		      DT_PROP(DT_INST_CHILD(n, display_timings), hback_porch) +                    \
		      DT_PROP(DT_INST_CHILD(n, display_timings), hfront_porch) +                   \
		      DT_PROP(DT_INST_CHILD(n, display_timings), hsync_len),                       \
	 .display_cyc = DT_INST_PROP(n, width),                                                    \
	 .back_porch = DT_PROP(DT_INST_CHILD(n, display_timings), hback_porch),                    \
	 .sync_width = DT_PROP(DT_INST_CHILD(n, display_timings), hsync_len),                      \
	 .sync_polarity = DT_PROP(DT_INST_CHILD(n, display_timings), hsync_active)}

#define RENESAS_RA_GLCDC_VTIMING(n)                                                                \
	{.total_cyc = DT_INST_PROP(n, height) +                                                    \
		      DT_PROP(DT_INST_CHILD(n, display_timings), vback_porch) +                    \
		      DT_PROP(DT_INST_CHILD(n, display_timings), vfront_porch) +                   \
		      DT_PROP(DT_INST_CHILD(n, display_timings), vsync_len),                       \
	 .display_cyc = DT_INST_PROP(n, height),                                                   \
	 .back_porch = DT_PROP(DT_INST_CHILD(n, display_timings), vback_porch),                    \
	 .sync_width = DT_PROP(DT_INST_CHILD(n, display_timings), vsync_len),                      \
	 .sync_polarity = DT_PROP(DT_INST_CHILD(n, display_timings), vsync_active)}

#define RENESAS_RA_GLCDC_OUTPUT_ENDIAN(n)                                                          \
	UTIL_CAT(DISPLAY_ENDIAN_, DT_INST_STRING_UPPER_TOKEN_OR(n, output_endian, LITTLE))

#define RENESAS_RA_GLCDC_OUTPUT_COLOR_ODER(n)                                                      \
	UTIL_CAT(DISPLAY_COLOR_ORDER_, DT_INST_STRING_UPPER_TOKEN_OR(n, output_color_oder, RGB))

#define RENESAS_RA_GLCDC_OUTPUT_DE_POLARITY(n)                                                     \
	UTIL_CAT(DISPLAY_SIGNAL_POLARITY_,                                                         \
		 DT_INST_STRING_UPPER_TOKEN_OR(n, output_data_signal_polarity, HIACTIVE))

#define RENESAS_RA_GLCDC_OUTPUT_SYNC_EDGE(n)                                                       \
	UTIL_CAT(DISPLAY_SIGNAL_SYNC_EDGE_,                                                        \
		 DT_INST_STRING_UPPER_TOKEN_OR(n, output_signal_sync_edge, FALLING))

#define RENESAS_RA_GLCDC_BG_COLOR(n)                                                               \
	{                                                                                          \
		.byte = {                                                                          \
			.a = DT_INST_PROP_OR(n, def_back_color_alpha, 255),                        \
			.r = DT_INST_PROP_OR(n, def_back_color_red, 255),                          \
			.g = DT_INST_PROP_OR(n, def_back_color_green, 255),                        \
			.b = DT_INST_PROP_OR(n, def_back_color_blue, 255)                          \
		}                                                                                  \
	}

#define RENESAS_RA_GLCDC_TCON_HSYNC_PIN(n)                                                         \
	UTIL_CAT(GLCDC_, DT_INST_STRING_UPPER_TOKEN_OR(n, output_pin_hsync, TCON_PIN_1))

#define RENESAS_RA_GLCDC_TCON_VSYNC_PIN(n)                                                         \
	UTIL_CAT(GLCDC_, DT_INST_STRING_UPPER_TOKEN_OR(n, output_pin_vsync, TCON_PIN_0))

#define RENESAS_RA_GLCDC_TCON_DE_PIN(n)                                                            \
	UTIL_CAT(GLCDC_, DT_INST_STRING_UPPER_TOKEN_OR(n, output_pin_de, TCON_PIN_2))

#define RENESAS_RA_GLCDC_OUTPUT_CLOCK_DIV(n)                                                       \
	UTIL_CAT(GLCDC_PANEL_CLK_DIVISOR_, DT_INST_PROP_OR(n, output_clock_divisor, 8))

#endif /* ZEPHYR_DRIVERS_DISPLAY_RENESAS_RA_H_ */

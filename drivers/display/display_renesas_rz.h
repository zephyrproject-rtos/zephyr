/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_RENESAS_RZ_H_
#define ZEPHYR_DRIVERS_DISPLAY_RENESAS_RZ_H_

#include <zephyr/drivers/display.h>

#define ROUND_UP_64BYTES(x)    ROUND_UP(x, NUM_BITS(uint64_t))
#define INPUT_FORMAT_PIXEL(n)  DT_INST_PROP(n, input_pixel_format)
#define OUTPUT_FORMAT_PIXEL(n) DT_INST_PROP(n, output_pixel_format)

#define RENESAS_RZ_LCDC_IN_PIXEL_FORMAT_1  (DISPLAY_IN_FORMAT_32BITS_RGB888)
#define RENESAS_RZ_LCDC_IN_PIXEL_FORMAT_8  (DISPLAY_IN_FORMAT_32BITS_ARGB8888)
#define RENESAS_RZ_LCDC_IN_PIXEL_FORMAT_16 (DISPLAY_IN_FORMAT_16BITS_RGB565)

#define RENESAS_RZ_LCDC_OUT_PIXEL_FORMAT_1  (DISPLAY_OUT_FORMAT_24BITS_RGB888)
#define RENESAS_RZ_LCDC_OUT_PIXEL_FORMAT_16 (DISPLAY_OUT_FORMAT_16BITS_RGB565)

#define RENESAS_RZ_DISPLAY_GET_PIXEL_FORMAT(n)                                                     \
	(DT_INST_PROP(n, input_pixel_format) == PANEL_PIXEL_FORMAT_RGB_888                         \
		 ? PANEL_PIXEL_FORMAT_ARGB_8888                                                    \
		 : DT_INST_PROP(n, input_pixel_format))

#define DISPLAY_HSIZE(n) (DT_INST_PROP(n, width))
#define DISPLAY_VSIZE(n) (DT_INST_PROP(n, height))

#define RENESAS_RZ_LCDC_IN_PIXEL_FORMAT(n)                                                         \
	UTIL_CAT(RENESAS_RZ_LCDC_IN_PIXEL_FORMAT_, INPUT_FORMAT_PIXEL(n))

#define RENESAS_RZ_LCDC_OUT_PIXEL_FORMAT(n)                                                        \
	UTIL_CAT(RENESAS_RZ_LCDC_OUT_PIXEL_FORMAT_, OUTPUT_FORMAT_PIXEL(n))

#define RENESAS_RZ_LCDC_PIXEL_BYTE_SIZE(n)                                                         \
	(DISPLAY_BITS_PER_PIXEL(RENESAS_RZ_DISPLAY_GET_PIXEL_FORMAT(n)) >> 3)

#define RENESAS_RZ_DISPLAY_BUFFER_HSTRIDE_BYTE(n)                                                  \
	(DISPLAY_HSIZE(n) * DISPLAY_BITS_PER_PIXEL(INPUT_FORMAT_PIXEL(n)) >> 3)

#define RENESAS_RZ_LCDC_HTIMING(n)                                                                 \
	{.total_cyc = DT_INST_PROP(n, width) +                                                     \
		      DT_PROP(DT_INST_CHILD(n, display_timings), hback_porch) +                    \
		      DT_PROP(DT_INST_CHILD(n, display_timings), hfront_porch),                    \
	 .display_cyc = DT_INST_PROP(n, width),                                                    \
	 .back_porch = DT_PROP(DT_INST_CHILD(n, display_timings), hback_porch),                    \
	 .sync_width = DT_PROP(DT_INST_CHILD(n, display_timings), hsync_len),                      \
	 .sync_polarity = DT_PROP(DT_INST_CHILD(n, display_timings), hsync_active)}

#define RENESAS_RZ_LCDC_VTIMING(n)                                                                 \
	{.total_cyc = DT_INST_PROP(n, height) +                                                    \
		      DT_PROP(DT_INST_CHILD(n, display_timings), vback_porch) +                    \
		      DT_PROP(DT_INST_CHILD(n, display_timings), vfront_porch),                    \
	 .display_cyc = DT_INST_PROP(n, height),                                                   \
	 .back_porch = DT_PROP(DT_INST_CHILD(n, display_timings), vback_porch),                    \
	 .sync_width = DT_PROP(DT_INST_CHILD(n, display_timings), vsync_len),                      \
	 .sync_polarity = DT_PROP(DT_INST_CHILD(n, display_timings), vsync_active)}

#define RENESAS_RZ_LCDC_OUTPUT_DE_POLARITY(n) DT_INST_ENUM_IDX(n, output_data_signal_polarity)

#define RENESAS_RZ_LCDC_OUTPUT_SYNC_EDGE(n) DT_INST_ENUM_IDX(n, output_signal_sync_edge)

#define RENESAS_RZ_LCDC_BG_COLOR(n)                                                                \
	{                                                                                          \
		.byte = {                                                                          \
			.a = DT_INST_PROP(n, background_color_alpha),                              \
			.r = DT_INST_PROP(n, background_color_red),                                \
			.g = DT_INST_PROP(n, background_color_green),                              \
			.b = DT_INST_PROP(n, background_color_blue)                                \
		}                                                                                  \
	}

#endif /* ZEPHYR_DRIVERS_DISPLAY_RENESAS_RZ_H_ */

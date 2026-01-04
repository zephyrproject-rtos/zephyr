/*
 * Copyright 2019, 2024-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "vglite_layer.h"
#ifndef STATIC_PATH_DEFINES_H
#define STATIC_PATH_DEFINES_H

#include "vg_lite.h"

typedef union data_mnemonic {
	int32_t cmd;
	int32_t data;
} data_mnemonic_t;

typedef struct path_info {
	uint32_t path_length;
	float bounding_box[4];
	int32_t *path_args;
	uint8_t *path_cmds;
	uint8_t end_path_flag;
} path_info_t;

typedef struct stroke_info {
	uint32_t dashPatternCnt;
	float dashPhase;
	float *dashPattern;
	float strokeWidth;
	float miterlimit;
	uint32_t strokeColor;
	vg_lite_cap_style_t linecap;
	vg_lite_join_style_t linejoin;
} stroke_info_t;

typedef struct image_info {
	char *image_name;
	int image_size[2];
	vg_lite_format_t data_format;
	float *transform;
	svg_node_t *render_sequence;
	int drawable_count;
	int path_count;
	stroke_info_t *stroke_info;
	uint8_t image_count;
	image_buf_data_t **raw_images;
	path_info_t paths_info[];
} image_info_t;

typedef struct stopValue {
	float offset;
	uint32_t stop_color;
} stopValue_t;

typedef struct linearGradient {
	uint32_t num_stop_points;
	vg_lite_linear_gradient_parameter_t linear_gradient;
	stopValue_t *stops;
} linearGradient_t;

typedef struct radialGradient {
	uint32_t num_stop_points;
	vg_lite_radial_gradient_parameter_t radial_gradient;
	stopValue_t *stops;
} radialGradient_t;

typedef struct hybridPath {
	fill_mode_t fillType;
	vg_lite_draw_path_type_t pathType;
} hybridPath_t;

typedef struct gradient_mode {
	linearGradient_t **linearGrads;
	radialGradient_t **radialGrads;
	hybridPath_t *hybridPath;
	vg_lite_fill_t *fillRule;
} gradient_mode_t;

typedef struct msg_glyph_item {
	uint16_t g_u16; /* Unicode number of glyph */
	uint16_t k;     /* Horizontal advancement for next glyph */
} msg_glyph_item_t;

typedef struct svg_text_string_data {
	int x, y;               /* Top,left co-ordinates of text region */
	int font_size;          /* font-size for text rendering */
	float *tmatrix;         /* Transform matrix for text area */
	uint32_t text_color;    /* Color value in ARGB format */
	font_info_t *font_face; /* Pointer to pre-defined font-face */
	int msg_len;            /* Length of message in number of unicode bytes */
	int direction;          /* Glyph layout direction */
				/*
				 * Supported: left->right
				 * Unsupported: right->left, top->bottom, bottom->top
				 */
	msg_glyph_item_t *msg_glyphs;
} svg_text_string_data_t;

typedef struct svg_text_info {
	int num_text_strings;
	svg_text_string_data_t *text_strings;
} svg_text_info_t;

#endif

/*path id=path826-3*/
static uint8_t HourNeedle_path_1_cmds[] = {VLC_OP_MOVE,  VLC_OP_CUBIC, VLC_OP_LINE,
					   VLC_OP_CUBIC, VLC_OP_CUBIC, VLC_OP_LINE,
					   VLC_OP_CUBIC, VLC_OP_LINE,  VLC_OP_END};

static int32_t HourNeedle_path_1_args[] = {
	-0.26, 115.57, -2.36, 115.57, -4.36, 113.57, -5.46, 109.47, -5.46, 10.87,  -3.86,
	11.67, -1.96,  12.17, -0.26,  12.17, 1.64,   12.17, 3.34,   11.77, 4.94,   10.87,
	4.94,  109.47, 3.84,  113.57, 1.74,  115.57, -0.26, 115.57, -0.26, 115.57,
};

/*path id=path848*/
static uint8_t HourNeedle_path_2_cmds[] = {VLC_OP_MOVE,  VLC_OP_CUBIC, VLC_OP_CUBIC, VLC_OP_CUBIC,
					   VLC_OP_CUBIC, VLC_OP_LINE,  VLC_OP_END};

static int32_t HourNeedle_path_2_args[] = {
	-0.26,  11.77,  5.94,  11.77,  11.14, 6.67,   11.14,  0.27,  11.14,  -6.03,
	6.14,   -11.23, -0.26, -11.23, -6.66, -11.23, -11.66, -6.03, -11.66, 0.27,
	-11.66, 6.57,   -6.66, 11.77,  -0.26, 11.77,  -0.26,  11.77,
};

/*path id=path851*/
static uint8_t HourNeedle_path_3_cmds[] = {VLC_OP_MOVE,  VLC_OP_CUBIC, VLC_OP_CUBIC, VLC_OP_CUBIC,
					   VLC_OP_CUBIC, VLC_OP_LINE,  VLC_OP_END};

static int32_t HourNeedle_path_3_args[] = {
	-0.26, 4.77,  2.44,  4.77,  4.54,  2.67,  4.54,  -0.03, 4.54,  -2.73,
	2.24,  -4.73, -0.26, -4.73, -2.76, -4.73, -5.06, -2.73, -5.06, -0.03,
	-5.06, 2.67,  -2.96, 4.77,  -0.26, 4.77,  -0.26, 4.77,
};

hybridPath_t HourNeedle_hybrid_path[] = {
	{.fillType = FILL_CONSTANT, .pathType = VG_LITE_DRAW_FILL_PATH},
	{.fillType = NO_FILL_MODE, .pathType = VG_LITE_DRAW_ZERO},
	{.fillType = FILL_CONSTANT, .pathType = VG_LITE_DRAW_FILL_PATH},
	{.fillType = NO_FILL_MODE, .pathType = VG_LITE_DRAW_ZERO},
	{.fillType = FILL_CONSTANT, .pathType = VG_LITE_DRAW_FILL_PATH},
	{.fillType = NO_FILL_MODE, .pathType = VG_LITE_DRAW_ZERO},

};

static vg_lite_fill_t HourNeedle_fill_rule[] = {VG_LITE_FILL_EVEN_ODD, VG_LITE_FILL_EVEN_ODD,
						VG_LITE_FILL_EVEN_ODD};

static gradient_mode_t HourNeedle_gradient_info = {.linearGrads = NULL,
						   .radialGrads = NULL,
						   .hybridPath = HourNeedle_hybrid_path,
						   .fillRule = HourNeedle_fill_rule};

static float HourNeedle_transform_matrix[] = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
					      1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
					      1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

static svg_node_t HourNeedle_render_sequence[] = {
	ePathNode,
	ePathNode,
	ePathNode,
};

static image_info_t HourNeedle = {
	.image_name = "HourNeedle",
	.image_size = {400, 400},
	.data_format = VG_LITE_S32,
	.transform = HourNeedle_transform_matrix,
	.render_sequence = HourNeedle_render_sequence,
	.drawable_count = 3,
	.path_count = 3,
	.stroke_info = NULL,
	.image_count = 0,
	.raw_images = (image_buf_data_t **)NULL,
	.text_info = NULL,
	.paths_info = {{.path_cmds = (uint8_t *)HourNeedle_path_1_cmds,
			.path_args = (int32_t *)HourNeedle_path_1_args,
			.path_length = sizeof(HourNeedle_path_1_cmds),
			.end_path_flag = 0,
			.bounding_box = {0.26, 10.87, 5.46, 115.57}},
		       {.path_cmds = (uint8_t *)HourNeedle_path_2_cmds,
			.path_args = (int32_t *)HourNeedle_path_2_args,
			.path_length = sizeof(HourNeedle_path_2_cmds),
			.end_path_flag = 0,
			.bounding_box = {0.26, 6.03, 11.66, 11.77}},
		       {.path_cmds = (uint8_t *)HourNeedle_path_3_cmds,
			.path_args = (int32_t *)HourNeedle_path_3_args,
			.path_length = sizeof(HourNeedle_path_3_cmds),
			.end_path_flag = 0,
			.bounding_box = {0.26, 2.67, 5.06, 4.77}}},
};

uint32_t HourNeedle_color_data[] = {0xff8a3523U, 0xff0b0d31U, 0xff41e8dfU};

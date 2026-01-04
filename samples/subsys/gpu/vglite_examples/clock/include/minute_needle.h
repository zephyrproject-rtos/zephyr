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

/*path id=path951*/
static uint8_t MinuteNeedle_path_1_cmds[] = {VLC_OP_MOVE,  VLC_OP_CUBIC, VLC_OP_CUBIC, VLC_OP_CUBIC,
					     VLC_OP_CUBIC, VLC_OP_LINE,  VLC_OP_END};

static int32_t MinuteNeedle_path_1_args[] = {
	-0.02,  12.30,  6.28,  12.30,  11.38, 7.20,   11.38,  0.90,  11.38,  -5.40,
	6.28,   -10.50, -0.02, -10.50, -6.32, -10.50, -11.42, -5.50, -11.42, 0.80,
	-11.42, 7.10,   -6.32, 12.30,  -0.02, 12.30,  -0.02,  12.30,
};

/*path id=path826-3-3*/
static uint8_t MinuteNeedle_path_2_cmds[] = {VLC_OP_MOVE,  VLC_OP_CUBIC, VLC_OP_LINE,
					     VLC_OP_CUBIC, VLC_OP_CUBIC, VLC_OP_LINE,
					     VLC_OP_CUBIC, VLC_OP_LINE,  VLC_OP_END};

static int32_t MinuteNeedle_path_2_args[] = {
	-0.02, 150.40, -2.02, 150.40, -4.12, 148.40, -5.22, 144.30, -5.22, 11.40,  -3.62,
	12.20, -1.82,  12.70, -0.02,  12.70, 1.78,   12.70, 3.58,   12.30, 5.18,   11.40,
	5.18,  144.30, 4.18,  148.40, 2.08,  150.40, -0.02, 150.40, -0.02, 150.40,
};

/*path id=path953*/
static uint8_t MinuteNeedle_path_3_cmds[] = {VLC_OP_MOVE,  VLC_OP_CUBIC, VLC_OP_CUBIC, VLC_OP_CUBIC,
					     VLC_OP_CUBIC, VLC_OP_LINE,  VLC_OP_END};

static int32_t MinuteNeedle_path_3_args[] = {
	0.19,  5.28,  2.79,  5.28,  4.89,  3.18,  4.89,  0.58,  4.89,  -2.02,
	2.79,  -4.12, 0.19,  -4.12, -2.41, -4.12, -4.51, -2.02, -4.51, 0.58,
	-4.51, 3.08,  -2.41, 5.28,  0.19,  5.28,  0.19,  5.28,
};

hybridPath_t MinuteNeedle_hybrid_path[] = {
	{.fillType = FILL_CONSTANT, .pathType = VG_LITE_DRAW_FILL_PATH},
	{.fillType = NO_FILL_MODE, .pathType = VG_LITE_DRAW_ZERO},
	{.fillType = FILL_CONSTANT, .pathType = VG_LITE_DRAW_FILL_PATH},
	{.fillType = NO_FILL_MODE, .pathType = VG_LITE_DRAW_ZERO},
	{.fillType = FILL_CONSTANT, .pathType = VG_LITE_DRAW_FILL_PATH},
	{.fillType = NO_FILL_MODE, .pathType = VG_LITE_DRAW_ZERO},

};

static vg_lite_fill_t MinuteNeedle_fill_rule[] = {VG_LITE_FILL_EVEN_ODD, VG_LITE_FILL_EVEN_ODD,
						  VG_LITE_FILL_EVEN_ODD};

static gradient_mode_t MinuteNeedle_gradient_info = {.linearGrads = NULL,
						     .radialGrads = NULL,
						     .hybridPath = MinuteNeedle_hybrid_path,
						     .fillRule = MinuteNeedle_fill_rule};

static float MinuteNeedle_transform_matrix[] = {
	1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

static svg_node_t MinuteNeedle_render_sequence[] = {
	ePathNode,
	ePathNode,
	ePathNode,
};

static image_info_t MinuteNeedle = {
	.image_name = "MinuteNeedle",
	.image_size = {400, 400},
	.data_format = VG_LITE_S32,
	.transform = MinuteNeedle_transform_matrix,
	.render_sequence = MinuteNeedle_render_sequence,
	.drawable_count = 3,
	.path_count = 3,
	.stroke_info = NULL,
	.image_count = 0,
	.raw_images = (image_buf_data_t **)NULL,
	.text_info = NULL,
	.paths_info = {{.path_cmds = (uint8_t *)MinuteNeedle_path_1_cmds,
			.path_args = (int32_t *)MinuteNeedle_path_1_args,
			.path_length = sizeof(MinuteNeedle_path_1_cmds),
			.end_path_flag = 0,
			.bounding_box = {0.02, 5.40, 11.42, 12.30}},
		       {.path_cmds = (uint8_t *)MinuteNeedle_path_2_cmds,
			.path_args = (int32_t *)MinuteNeedle_path_2_args,
			.path_length = sizeof(MinuteNeedle_path_2_cmds),
			.end_path_flag = 0,
			.bounding_box = {0.02, 11.40, 5.22, 150.40}},
		       {.path_cmds = (uint8_t *)MinuteNeedle_path_3_cmds,
			.path_args = (int32_t *)MinuteNeedle_path_3_args,
			.path_length = sizeof(MinuteNeedle_path_3_cmds),
			.end_path_flag = 0,
			.bounding_box = {0.19, 2.02, 4.89, 5.28}}},
};

uint32_t MinuteNeedle_color_data[] = {0xff0b0d31U, 0xffe2d945U, 0xff41e8dfU};

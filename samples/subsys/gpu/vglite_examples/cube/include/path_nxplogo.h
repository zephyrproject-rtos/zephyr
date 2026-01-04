/*
 * Copyright 2023-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __NXP_PATHS_H__
#define __NXP_PATHS_H__

#include "vg_lite.h"

#define _S8 (signed char)

/* nxp_path_data0 ===========================================*/
signed char nxp_path_data0[] = {
	2, _S8 0.0f,    _S8 0.0f,
	4, _S8 7.49f,   _S8 0.0f,
	4, _S8 20.066f, _S8 14.721f,
	4, _S8 20.066f, _S8 0.0f,
	4, _S8 27.556f, _S8 0.0f,
	4, _S8 27.556f, _S8 24.457f,
	4, _S8 20.066f, _S8 24.457f,
	4, _S8 7.49f,   _S8 9.748f,
	4, _S8 7.49f,   _S8 24.457f,
	4, _S8 0.0f,    _S8 24.457f,
	0,
};
signed char nxp_path_data1[] = {
	2, _S8 20.066f, _S8 0.0f,
	4, _S8 28.931f, _S8 0.0f,
	4, _S8 33.958f, _S8 8.051f,
	4, _S8 38.993f, _S8 0.0f,
	4, _S8 47.777f, _S8 0.0f,
	4, _S8 40.437f, _S8 12.188f,
	4, _S8 47.777f, _S8 24.457f,
	4, _S8 38.993f, _S8 24.457f,
	4, _S8 33.958f, _S8 16.504f,
	4, _S8 28.931f, _S8 24.457f,
	4, _S8 20.066f, _S8 24.457f,
	4, _S8 27.556f, _S8 12.188f,
	0,
};
signed char nxp_path_data2[] = {
	2, _S8 40.437f, _S8 0.0f,
	4, _S8 63.042f, _S8 0.0f,
	8, _S8 72.242f,  _S8 0.0f,   _S8 72.242f, _S8 19.107f, _S8 63.042f, _S8 19.107f,
	4, _S8 49.672f, _S8 19.107f,
	4, _S8 49.672f, _S8 24.457f,
	4, _S8 40.437f, _S8 24.457f,
	4, _S8 40.437f, _S8 0.0f,

	2, _S8 49.672f, _S8 6.114f,
	4, _S8 59.576f, _S8 6.114f,
	8, _S8 62.12f,  _S8 6.114f,  _S8 62.12f, _S8 13.044f, _S8 59.576f, _S8 13.044f,
	4, _S8 49.672f, _S8 13.044f,
	0,
};
signed char nxp_path_data3[] = {
	2, _S8 20.066f, _S8 0.0f,
	4, _S8 27.556f, _S8 0.0f,
	4, _S8 27.556f, _S8 24.457f,
	4, _S8 20.066f, _S8 24.457f,
	4, _S8 27.556f, _S8 12.188f,
	2, _S8 20.066f, _S8 0.0f,
	0,
};
signed char nxp_path_data4[] = {
	2, _S8 40.437f, _S8 0.0f,
	4, _S8 47.777f, _S8 0.0f,
	4, _S8 40.437f, _S8 12.188f,
	4, _S8 47.777f, _S8 24.457f,
	4, _S8 40.437f, _S8 24.457f,
	4, _S8 40.437f, _S8 0.0f,
	0,
};
#undef _S8

vg_lite_path_t nxp_path[5] = {
	{
		{0, 0, 75, 25},	/* left, top, right,bottom */
		VG_LITE_HIGH,		/* quality */
		VG_LITE_S8,		/* -128 to 127 coordinate range */
		{0},			/* uploaded */
		sizeof(nxp_path_data0),	/* path length */
		nxp_path_data0,		/* path data */
		1		/* initially, path is changed for uploaded*/
	},
	{{0, 0, 75, 25}, VG_LITE_HIGH, VG_LITE_S8, {0},
	sizeof(nxp_path_data1), nxp_path_data1, 1},
	{{0, 0, 75, 25}, VG_LITE_HIGH, VG_LITE_S8, {0},
	sizeof(nxp_path_data2), nxp_path_data2, 1},
	{{0, 0, 75, 25}, VG_LITE_HIGH, VG_LITE_S8, {0},
	sizeof(nxp_path_data3), nxp_path_data3, 1},
	{{0, 0, 75, 25}, VG_LITE_HIGH, VG_LITE_S8, {0},
	sizeof(nxp_path_data4), nxp_path_data4, 1},
};

uint32_t nxp_color_data[] = {
	/* nxp_path_data0 */   0xff00b5f9U,
	/* nxp_path_data1 */   0xffdbb17bU,
	/* nxp_path_data2 */   0xff00d2c9U,
	/* nxp_path_data3 */   0xff378495U,
	/* nxp_path_data4 */   0xff339873U,
};

int nxp_path_count = 5;

#endif	/* __NXP_PATHS_H__ */

/*
 * Copyright 2023-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "draw_clock.h"

#include "bg_paths.h"
#include "h_paths.h"
#include "m_paths.h"
#include "s_paths.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

void cleanup(vg_lite_path_t *path, uint8_t path_count)
{
	uint8_t i;

	for (i = 0; i < path_count; i++) {
		vg_lite_clear_path(&path[i]);
	}

	vg_lite_close();
}

int draw_clock(vg_lite_buffer_t *buffer, vg_lite_color_t back_color)
{
	vg_lite_error_t error = VG_LITE_SUCCESS;
	vg_lite_matrix_t matrix;
	uint8_t count;
	static float scale_ratio = 0.377f;
	static float angle = 0.0f;

	/* Draw the path using the matrix. */
	vg_lite_clear(buffer, NULL, back_color);
	vg_lite_identity(&matrix);
	vg_lite_translate(10, 0, &matrix);
	vg_lite_scale(scale_ratio, scale_ratio, &matrix);

	for (count = 0; count < bg_count; count++) {
		error = vg_lite_draw(buffer, &bg_data[count], VG_LITE_FILL_NON_ZERO, &matrix,
				     VG_LITE_BLEND_SRC_OVER, bg_color[count]);
		if (error) {
			cleanup(bg_data, count);
			return -1;
		}
	}

	vg_lite_identity(&matrix);
	vg_lite_translate(10, 0, &matrix);

	vg_lite_translate(311 * scale_ratio, 311 * scale_ratio, &matrix);
	vg_lite_rotate(angle, &matrix);
	vg_lite_translate(-311 * scale_ratio, -311 * scale_ratio, &matrix);

	vg_lite_scale(scale_ratio, scale_ratio, &matrix);

	for (count = 0; count < h_count; count++) {
		error = vg_lite_draw(buffer, &h_data[count], VG_LITE_FILL_NON_ZERO, &matrix,
				     VG_LITE_BLEND_SRC_OVER, h_color[count]);
		if (error) {
			cleanup(h_data, count);
			return -1;
		}
	}

	vg_lite_identity(&matrix);
	vg_lite_translate(10, 0, &matrix);

	vg_lite_translate(311 * scale_ratio, 311 * scale_ratio, &matrix);
	vg_lite_rotate(10 * angle, &matrix);
	vg_lite_translate(-311 * scale_ratio, -311 * scale_ratio, &matrix);

	vg_lite_scale(scale_ratio, scale_ratio, &matrix);

	for (count = 0; count < m_count; count++) {
		error = vg_lite_draw(buffer, &m_data[count], VG_LITE_FILL_NON_ZERO, &matrix,
				     VG_LITE_BLEND_SRC_OVER, m_color[count]);
		if (error) {
			cleanup(m_data, count);
			return -1;
		}
	}

	vg_lite_identity(&matrix);
	vg_lite_translate(10, 0, &matrix);

	vg_lite_translate(311 * scale_ratio, 311 * scale_ratio, &matrix);
	vg_lite_rotate(10 * 10 * angle, &matrix);
	vg_lite_translate(-311 * scale_ratio, -311 * scale_ratio, &matrix);

	vg_lite_scale(scale_ratio, scale_ratio, &matrix);

	for (count = 0; count < s_count; count++) {
		error = vg_lite_draw(buffer, &s_data[count], VG_LITE_FILL_NON_ZERO, &matrix,
				     VG_LITE_BLEND_SRC_OVER, s_color[count]);
		if (error) {
			cleanup(s_data, count);
			return -1;
		}
	}

	angle += 0.05f;
	return 1;
}

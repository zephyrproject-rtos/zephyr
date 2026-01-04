/*
 * Copyright 2024-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "vglite_layer.h"

#define QOI_MALLOC(sz) vg_lite_os_malloc(sz)
#define QOI_FREE(p)    vg_lite_os_free(p)
#define QOI_IMPLEMENTATION
#define QOI_NO_STDIO
#include "qoi.h"

/* GLobals */
static gradient_cache_entry_t *g_grad_cache;
static vg_lite_uint32_t g_chip_id;
#define MAX_FONT_CACHE_SZ (27 * 2)
typedef struct font_glyph_cache_entry {
	uint16_t u16;
	void *path_data;
	int hx; /* horiz-adv-x */
	vg_lite_path_t handle;
} font_glyph_cache_entry_t;

#ifndef TRANSPARENT_VGLITE_COLOUR
/* VGLITE Internal color format is ABGR */
#define TRANSPARENT_VGLITE_COLOUR(a, r, g, b)                                                      \
	((uint32_t)(a) << 24) | ((uint32_t)(b) << 16) | ((uint32_t)(g) << 8) | (uint32_t)r
#endif

/* Space horizontal advancement*/
#define SPACE_HX 278

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
vg_lite_error_t layer_create_image_buffers(UILayers_t *layer);
vg_lite_error_t layer_draw_images(vg_lite_buffer_t *rt, UILayers_t *layer,
				  vg_lite_matrix_t *transform_matrix);
vg_lite_error_t layer_free_images(UILayers_t *layer);

static uint32_t ARGB_2_VGLITE_COLOR(uint32_t x)
{
	uint32_t r, g, b, a;

	a = ((x & 0xFF000000U) >> 24);
	r = ((x & 0x00FF0000U) >> 16);
	g = ((x & 0x0000FF00U) >> 8);
	b = ((x & 0x000000FFU));

	return TRANSPARENT_VGLITE_COLOUR(a, r, g, b);
}

void matrix_multiply(vg_lite_matrix_t *result, const vg_lite_matrix_t *mat1,
		     const vg_lite_matrix_t *mat2)
{
	int row, col, k;

	/* Only compute the top two rows (0 and 1) for 2D affine transform */
	for (row = 0; row < 2; row++) {
		for (col = 0; col < 3; col++) {
			result->m[row][col] = 0.0f;
			for (k = 0; k < 3; k++) {
				result->m[row][col] += mat1->m[row][k] * mat2->m[k][col];
			}
		}
	}

	/* Set the fixed bottom row for 2D affine transform */
	result->m[2][0] = 0.0f;
	result->m[2][1] = 0.0f;
	result->m[2][2] = 1.0f;
}

static int is_matrix_identical(vg_lite_matrix_t *m1, vg_lite_matrix_t *m2)
{
	int row;

	for (row = 0; row < 3; row++) {
		if (m1->m[row][0] != m2->m[row][0]) {
			return 0;
		}
		if (m1->m[row][1] != m2->m[row][1]) {
			return 0;
		}
		if (m1->m[row][2] != m2->m[row][2]) {
			return 0;
		}
	}
	return 1;
}

vg_lite_error_t gradient_cache_init(void)
{
	g_grad_cache = (gradient_cache_entry_t *)vg_lite_os_malloc(MAX_GRADIENT_CACHE *
								   sizeof(gradient_cache_entry_t));
	if (g_grad_cache == NULL) {
		printk("Error: Memory allocation failed for g_grad_cache!\n");
		return VG_LITE_OUT_OF_MEMORY;
	}
	memset(g_grad_cache, 0, MAX_GRADIENT_CACHE * sizeof(gradient_cache_entry_t));

	return VG_LITE_SUCCESS;
}

void gradient_cache_free(void)
{
	int i;

	if (g_grad_cache != NULL) {
		for (i = 0; i < MAX_GRADIENT_CACHE; i++) {
			if (g_grad_cache[i].g != NULL) {
				if (g_grad_cache[i].type == eLinearGradientCacheEntry &&
				    g_chip_id != 0x255) {
					vg_lite_clear_linear_grad(
						&g_grad_cache[i].grad_data.lg.lGradient);
				} else if (g_grad_cache[i].type == eRadialGradientCacheEntry &&
					   g_chip_id != 0x255) {
					vg_lite_clear_radial_grad(
						&g_grad_cache[i].grad_data.rg.rGradient);
				} else if (g_grad_cache[i].type == eLinearGradientCacheEntry &&
					   g_chip_id == 0x255) {
					vg_lite_clear_grad(
						&g_grad_cache[i]
							 .grad_data.linear_basic.basic_gradient);
				} else {
					/*
					 * Unexpected gradient type or chip ID combination.
					 * This could be:
					 * - Radial gradient on chip 0x255 (not supported)
					 * - Unknown gradient type
					 * - Uninitialized cache entry
					 */
					printk("Warning: Unhandled gradient cache entry: "
					       "type=%d, chip_id=0x%X\n",
					       g_grad_cache[i].type, g_chip_id);
				}
			}
		}
		vg_lite_os_free(g_grad_cache);
		g_grad_cache = NULL;
	}
}

static vg_lite_error_t get_gpu_chip_info(void)
{
	vg_lite_uint32_t chip_rev = 0;

	/* Get product info and pass g_chip_id as a pointer */
	vg_lite_get_product_info(NULL, &g_chip_id, &chip_rev);

	/* Check g_chip_id for supported values (0x255, 0x355, 0x555) */
	if (g_chip_id == 0x255 || g_chip_id == 0x355 || g_chip_id == 0x555) {
		return VG_LITE_SUCCESS;
	}

	/* Print error messages for mismatch */
	printf("\r\nVGLite API initialization Error!!!\nHardware ChipId: 0x%X \r\n\r\n",
			g_chip_id);
	printf("\r\nNOT match vg_lite_options.h CHIPID: 0x%X\r\n\r\n", g_chip_id);

	return VG_LITE_NOT_SUPPORT;
}

/* Updated function to prepare colors/stops for legacy API */
static void prepare_legacy_gradient_data(const vg_lite_color_ramp_t *ramp, int count,
					 vg_lite_uint32_t *colors, vg_lite_uint32_t *stops)
{
	for (int i = 0; i < count; i++) {
		stops[i] = (vg_lite_uint32_t)(ramp[i].stop * 255.0f);
		colors[i] = ((uint32_t)(ramp[i].alpha * 255.0f) << 24) |
			    ((uint32_t)(ramp[i].red * 255.0f) << 16) |
			    ((uint32_t)(ramp[i].green * 255.0f) << 8) |
			    (uint32_t)(ramp[i].blue * 255.0f);
	}
}

void _gradient_stop_color_to_vglite_color(int32_t num_stop_points, stopValue_t *stops,
					  vg_lite_color_ramp_t *vgColorRamp)
{
	int i;

	for (i = 0; i < num_stop_points && i < MAX_GRADIENT_STOP_POINTS; i++) {
		vgColorRamp[i].stop = stops[i].offset;
		vgColorRamp[i].red = ((stops[i].stop_color & 0x00FF0000U) >> 16) / 255.0f;
		vgColorRamp[i].green = ((stops[i].stop_color & 0x0000FF00U) >> 8) / 255.0f;
		vgColorRamp[i].blue = (stops[i].stop_color & 0x000000FFU) / 255.0f;
		vgColorRamp[i].alpha = ((stops[i].stop_color & 0xFF000000U) >> 24) / 255.0f;
	}
}

static vg_lite_error_t setup_linear_gradient_legacy(gradient_cache_entry_t *cachedGradient,
						     linearGradient_t *gradient,
						     vg_lite_matrix_t *transform_matrix)
{
	vg_lite_error_t error;
	vg_lite_uint32_t colors[MAX_GRADIENT_STOP_POINTS];
	vg_lite_uint32_t stops[MAX_GRADIENT_STOP_POINTS];

	memset(&cachedGradient->grad_data.linear_basic.basic_gradient, 0,
	       sizeof(vg_lite_linear_gradient_t));
	error = vg_lite_init_grad(&cachedGradient->grad_data.linear_basic.basic_gradient);
	if (error) {
		printk("\r\nERROR: vg_lite_init_grad() failed (err=%d)!\r\n\r\n", error);
		return error;
	}

	_gradient_stop_color_to_vglite_color(gradient->num_stop_points, gradient->stops,
					     cachedGradient->vgColorRamp);

	prepare_legacy_gradient_data(cachedGradient->vgColorRamp, gradient->num_stop_points,
				     colors, stops);

	cachedGradient->grad_data.linear_basic.basic_gradient.count = gradient->num_stop_points;
	error = vg_lite_set_grad(&cachedGradient->grad_data.linear_basic.basic_gradient,
				 cachedGradient->grad_data.linear_basic.basic_gradient.count,
				 colors, stops);
	if (error != VG_LITE_SUCCESS) {
		return error;
	}

	if (is_matrix_identical(&cachedGradient->grad_data.linear_basic.basic_gradient.matrix,
				transform_matrix) == 0) {
		cachedGradient->grad_data.linear_basic.basic_gradient.matrix = *transform_matrix;
		error = vg_lite_update_grad(&cachedGradient->grad_data.linear_basic.basic_gradient);
	}

	return error;
}

static vg_lite_error_t setup_linear_gradient_extended(gradient_cache_entry_t *cachedGradient,
						       linearGradient_t *gradient,
						       vg_lite_matrix_t *transform_matrix)
{
	vg_lite_error_t error;

	_gradient_stop_color_to_vglite_color(gradient->num_stop_points, gradient->stops,
					     cachedGradient->vgColorRamp);

	cachedGradient->grad_data.lg.lGradient.count = gradient->num_stop_points;
	cachedGradient->grad_data.lg.params = gradient->linear_gradient;
	error = vg_lite_set_linear_grad(&cachedGradient->grad_data.lg.lGradient,
					cachedGradient->grad_data.lg.lGradient.count,
					cachedGradient->vgColorRamp,
					cachedGradient->grad_data.lg.params,
					VG_LITE_GRADIENT_SPREAD_PAD, 1);
	if (error != VG_LITE_SUCCESS) {
		return error;
	}

	if (is_matrix_identical(&cachedGradient->grad_data.lg.lGradient.matrix,
				transform_matrix) == 0) {
		cachedGradient->grad_data.lg.lGradient.matrix = *transform_matrix;
		error = vg_lite_update_linear_grad(&cachedGradient->grad_data.lg.lGradient);
	}

	return error;
}

static vg_lite_error_t setup_radial_gradient(gradient_cache_entry_t *cachedGradient,
					      radialGradient_t *gradient,
					      vg_lite_matrix_t *transform_matrix)
{
	vg_lite_error_t error;

	_gradient_stop_color_to_vglite_color(gradient->num_stop_points, gradient->stops,
					     cachedGradient->vgColorRamp);

	cachedGradient->grad_data.rg.rGradient.count = gradient->num_stop_points;
	cachedGradient->grad_data.rg.params = gradient->radial_gradient;

	error = vg_lite_set_radial_grad(&cachedGradient->grad_data.rg.rGradient,
					cachedGradient->grad_data.rg.rGradient.count,
					cachedGradient->vgColorRamp,
					cachedGradient->grad_data.rg.params,
					VG_LITE_GRADIENT_SPREAD_PAD, 1);
	if (error != VG_LITE_SUCCESS) {
		return error;
	}

	if (is_matrix_identical(&cachedGradient->grad_data.rg.rGradient.matrix,
				transform_matrix) == 0) {
		cachedGradient->grad_data.rg.rGradient.matrix = *transform_matrix;
		error = vg_lite_update_rad_grad(&cachedGradient->grad_data.rg.rGradient);
	}

	return error;
}

static void clear_cached_gradient(gradient_cache_entry_t *cachedGradient)
{
	if (cachedGradient->type == eLinearGradientCacheEntry && g_chip_id != 0x255) {
		vg_lite_clear_linear_grad(&cachedGradient->grad_data.lg.lGradient);
	} else if (cachedGradient->type == eRadialGradientCacheEntry && g_chip_id != 0x255) {
		vg_lite_clear_radial_grad(&cachedGradient->grad_data.rg.rGradient);
	} else if (cachedGradient->type == eLinearGradientCacheEntry && g_chip_id == 0x255) {
		vg_lite_clear_grad(&cachedGradient->grad_data.linear_basic.basic_gradient);
	} else {
		printk("Warning: Unhandled gradient cache entry: type=%d, chip_id=0x%X\n",
		       cachedGradient->type, g_chip_id);
	}
}

static int find_gradient_in_cache(void *grad)
{
	for (int i = 0; i < MAX_GRADIENT_CACHE; i++) {
		if (g_grad_cache[i].g == grad) {
			return i;
		}
	}
	return -1;
}

static int find_unused_cache_slot(void)
{
	for (int i = 1; i < MAX_GRADIENT_CACHE; i++) {
		if (g_grad_cache[i].g == NULL) {
			return i;
		}
	}
	return -1;
}

static vg_lite_error_t setup_gradient_by_type(gradient_cache_entry_t *cachedGradient,
					       void *grad, int type,
					       vg_lite_matrix_t *transform_matrix)
{
	if (type == eLinearGradientCacheEntry && g_chip_id != 0x255) {
		return setup_linear_gradient_extended(cachedGradient,
						      (linearGradient_t *)grad,
						      transform_matrix);
	}

	if (type == eRadialGradientCacheEntry && g_chip_id != 0x255) {
		return setup_radial_gradient(cachedGradient, (radialGradient_t *)grad,
					      transform_matrix);
	}

	if (type == eLinearGradientCacheEntry && g_chip_id == 0x255) {
		return setup_linear_gradient_legacy(cachedGradient, (linearGradient_t *)grad,
						    transform_matrix);
	}

	if (type == eRadialGradientCacheEntry && g_chip_id == 0x255) {
		printk("Error: Radial gradient is not supported for ChipId: 0x%X\r\n", g_chip_id);
		return VG_LITE_NOT_SUPPORT;
	}

	printk("Error: Invalid gradient configuration: type=%d, chip_id=0x%X\r\n",
	       type, g_chip_id);
	return VG_LITE_INVALID_ARGUMENT;
}

vg_lite_error_t gradient_cache_find(void *grad, int type, vg_lite_matrix_t *transform_matrix,
				    gradient_cache_entry_t **ppcachedEntry)
{
	vg_lite_error_t error;
	int cache_idx;
	gradient_cache_entry_t *cachedGradient = NULL;

	*ppcachedEntry = NULL;

	if (grad == NULL) {
		return VG_LITE_INVALID_ARGUMENT;
	}

	if (!(type == eLinearGradientCacheEntry || type == eRadialGradientCacheEntry)) {
		return VG_LITE_INVALID_ARGUMENT;
	}

	/* Check if gradient already exists in cache */
	cache_idx = find_gradient_in_cache(grad);
	if (cache_idx >= 0) {
		*ppcachedEntry = &g_grad_cache[cache_idx];
		return VG_LITE_SUCCESS;
	}

	/* Find unused cache slot */
	cache_idx = find_unused_cache_slot();
	if (cache_idx == -1) {
		return VG_LITE_OUT_OF_MEMORY;
	}

	cachedGradient = &g_grad_cache[cache_idx];

	/* Clear previous gradient if exists */
	clear_cached_gradient(cachedGradient);

	/* Setup new gradient based on type and chip ID */
	error = setup_gradient_by_type(cachedGradient, grad, type, transform_matrix);
	if (error != VG_LITE_SUCCESS) {
		return error;
	}

	/* Update cache entry */
	cachedGradient->g = grad;
	cachedGradient->type = (GradientCacheEntry_t)type;
	*ppcachedEntry = cachedGradient;

	return VG_LITE_SUCCESS;
}

vg_lite_error_t layer_create_image_buffers(UILayers_t *layer)
{
	vg_lite_error_t err;
	int image_count = layer->img_info->image_count;

	if (image_count > 0) {
		qoi_desc desc;
		int channels = 4;

		layer->dst_images = (vg_lite_buffer_t *)vg_lite_os_malloc(image_count *
									  sizeof(vg_lite_buffer_t));
		if (layer->dst_images == NULL) {
			printk("\r\nERROR: Failed to initialize memory descriptors!\r\n");
			return VG_LITE_OUT_OF_MEMORY;
		}

		layer->dst_img_data =
			(uint8_t **)vg_lite_os_malloc(image_count * sizeof(uint8_t *));
		if (layer->dst_img_data == NULL) {
			printk("\r\nERROR: Failed to initialize QOI decode memory pointers!\r\n");
			return VG_LITE_OUT_OF_MEMORY;
		}

		for (int j = 0; j < image_count; j++) {
			image_buf_data_t *raw_img = layer->img_info->raw_images[j];
			vg_lite_buffer_t *dst_img = &layer->dst_images[j];

			if (raw_img->format != VG_LITE_RGBA8888) {
				uint8_t *decomp_data = qoi_decode(raw_img->raw_data, raw_img->size,
								  &desc, channels);
				if (!decomp_data) {
					printk("\r\nERROR: Decompression is failed!\r\n\r\n");
					return VG_LITE_INVALID_ARGUMENT;
				}
				layer->dst_img_data[j] = decomp_data;
			} else {
				layer->dst_img_data[j] = raw_img->raw_data;
			}

			memset(dst_img, 0, sizeof(vg_lite_buffer_t));
			dst_img->width = raw_img->img_width;
			dst_img->height = raw_img->img_height;
			dst_img->stride = raw_img->img_width * 4;
			dst_img->format = VG_LITE_RGBA8888;

			err = vg_lite_allocate(dst_img);
			if (err != VG_LITE_SUCCESS) {
				printk("ERROR: Failed to allocate RGBA8888 image buffer "
				       "(err=%d)!\r\n",
				       err);
				return err;
			}

			dst_img->memory = layer->dst_img_data[j];
			dst_img->address = (vg_lite_uint32_t)layer->dst_img_data[j];
		}
	}

	return VG_LITE_SUCCESS;
}

vg_lite_error_t layer_draw_images(vg_lite_buffer_t *rt, UILayers_t *layer,
				  vg_lite_matrix_t *transform_matrix)
{
	vg_lite_error_t error;
	vg_lite_matrix_t tmatrix;

	for (int j = 0; j < layer->img_info->image_count; j++) {
		image_buf_data_t *raw_img = layer->img_info->raw_images[j];

		matrix_multiply(&tmatrix, transform_matrix, &raw_img->matrix);
		error = vg_lite_blit(rt, &layer->dst_images[j], &tmatrix, VG_LITE_BLEND_NONE, 0,
				     VG_LITE_FILTER_BI_LINEAR);
		if (error != VG_LITE_SUCCESS) {
			printk("Error: vg_lite_blit() failed! %d\r\n", error);
			return error;
		}
	}

	return VG_LITE_SUCCESS;
}

vg_lite_error_t layer_free_images(UILayers_t *layer)
{
	vg_lite_error_t error;

	if (layer->dst_images != NULL) {

		for (int j = 0; j < layer->img_info->image_count; j++) {
			image_buf_data_t *raw_img = layer->img_info->raw_images[j];
			vg_lite_buffer_t *dst_img = &layer->dst_images[j];

			error = vg_lite_free(dst_img);
			if (error != VG_LITE_SUCCESS) {
				printk("Error: Failed to release memory. error=%d (%p)!\r\n", error,
				       dst_img);
			}

			if (raw_img->raw_decode_data != NULL) {
				vg_lite_os_free(raw_img->raw_decode_data);
			}
			if (raw_img->format != VG_LITE_RGBA8888) {
				if (layer->dst_img_data != NULL) {
					/* Release memory for decoded QOI image */
					vg_lite_os_free(layer->dst_img_data[j]);
					layer->dst_img_data = NULL;
				}
			}
		}
		vg_lite_os_free(layer->dst_images);
		/* Release memory pointer holder for decoded QOI image */
		if (layer->dst_img_data != NULL) {
			vg_lite_os_free(layer->dst_img_data);
		}
		vg_lite_os_free(layer->dst_images);
		layer->dst_images = NULL;
	}

	return VG_LITE_SUCCESS;
}

static font_glyph_cache_entry_t *g_font_glyph_cache;

vg_lite_error_t layer_font_glyph_cache_init(void)
{
	g_font_glyph_cache =
		vg_lite_os_malloc(sizeof(font_glyph_cache_entry_t) * MAX_FONT_CACHE_SZ);
	if (g_font_glyph_cache == NULL) {
		printk("\r\nERROR: Failed to initialize memory descriptors!\r\n");
		return VG_LITE_OUT_OF_MEMORY;
	}
	memset(g_font_glyph_cache, 0, sizeof(font_glyph_cache_entry_t) * MAX_FONT_CACHE_SZ);
	return VG_LITE_SUCCESS;
}

int layer_font_glyph_cache_free(void)
{
	if (g_font_glyph_cache != NULL) {
		vg_lite_os_free(g_font_glyph_cache);
		g_font_glyph_cache = NULL;
	}
	return VG_LITE_SUCCESS;
}

vg_lite_error_t layer_font_glyph_cache_find(font_info_t *font, uint16_t g_u16,
					    font_glyph_cache_entry_t **handle,
					    uint32_t text_data_size)
{
	vg_lite_error_t vg_err = VG_LITE_SUCCESS;
	int gIdx = -1, cacheIdx = -1, i = 0;
	int space_gIdx = font->max_glyphs;
	/* Reset output parameter */
	*handle = NULL;

	/* Find glyph for unicode number */
	for (gIdx = 0; gIdx < font->max_glyphs; gIdx++) {
		if (font->unicode[gIdx] == 0x20) {
			space_gIdx = gIdx;
		} else if (g_u16 == font->unicode[gIdx]) {
			if (gIdx > space_gIdx) {
				gIdx = gIdx - 1;
			}
			break;
		} else {
			/* Current glyph doesn't match - continue searching */
		}
	}

	if (gIdx == font->max_glyphs) {
		printk("ERROR: glyph=[%04x] not found ", g_u16);
		/* TODO: It is possible to share missing-glyph for given font. */
		return VG_LITE_INVALID_ARGUMENT;
	}

	/* Lookup cache if entry exits */
	for (i = 0; i < MAX_FONT_CACHE_SZ; i++) {
		if (g_u16 == g_font_glyph_cache[i].u16) {
			*handle = &g_font_glyph_cache[i];
			return VG_LITE_SUCCESS;
		}
	}

	/* Lookup empty entry in cache table */
	for (i = 0; i < MAX_FONT_CACHE_SZ; i++) {
		if (g_font_glyph_cache[i].path_data == NULL) {
			break;
		}
	}
	if (i == MAX_FONT_CACHE_SZ) {
		printk("ERROR: font-cache exhausted %d", g_u16);
		/* TODO: It is possible recycle cache entries using LRU */
		return VG_LITE_OUT_OF_MEMORY;
	}

	/* Create handle from glyph path_info */
	cacheIdx = i;
	text_data_size = vg_lite_get_path_length(font->gi[gIdx].path_cmds,
						 font->gi[gIdx].path_length, VG_LITE_S32);
	vg_err = vg_lite_init_path(&g_font_glyph_cache[cacheIdx].handle, VG_LITE_S32,
				   VG_LITE_MEDIUM, text_data_size, NULL, 0, 0, 0, 0);
	if (vg_err != VG_LITE_SUCCESS) {
		printk("\r\nERROR: Failed to initialize graphic artifacts!\r\n\r\n");
		return vg_err;
	}
	vg_err = vg_lite_append_path(&g_font_glyph_cache[cacheIdx].handle, font->gi[gIdx].path_cmds,
				     font->gi[gIdx].path_args, font->gi[gIdx].path_length);
	if (vg_err != VG_LITE_SUCCESS) {
		printk("\r\nERROR: vg_lite_append_path Failed Couldn't append path data!\r\n\r\n");
		return vg_err;
	}
	/* Adjust end-path configuration */
	g_font_glyph_cache[cacheIdx].handle.add_end = font->gi[gIdx].end_path_flag;
	/* - Create path from path_info - ends */

	/* return newly created path handle */
	g_font_glyph_cache[i].path_data = g_font_glyph_cache[cacheIdx].handle.path;
	g_font_glyph_cache[i].u16 = g_u16;
	g_font_glyph_cache[i].hx = font->gi[gIdx].hx;
	*handle = &g_font_glyph_cache[cacheIdx];

	return VG_LITE_SUCCESS;
}

vg_lite_error_t layer_draw_text(vg_lite_buffer_t *rt, UILayers_t *layer,
				vg_lite_matrix_t *transform_matrix, int i)
{
	svg_text_info_t *ti = layer->img_info->text_info;
	vg_lite_error_t vg_err = VG_LITE_SUCCESS;
	const int matrix_size_in_float = 9;

	/* Count total number of glyphs */
	svg_text_string_data_t *td = NULL;
	font_info_t *font = NULL;
	vg_lite_matrix_t tmatrix;
	uint32_t text_data_size = 0;

	td = &ti->text_strings[i];
	font = td->font_face;
	matrix_multiply(&tmatrix, transform_matrix,
			(const vg_lite_matrix_t *)&td->tmatrix[i * matrix_size_in_float]);

	vg_lite_translate(td->x, td->y, &tmatrix);

	float font_scale_factor = (float)td->font_size / font->units_per_em;

	vg_lite_scale(font_scale_factor, -font_scale_factor, &tmatrix);
	for (int j = 0; j < td->msg_len; j++) {
		font_glyph_cache_entry_t *glyph = NULL; /* Glyph handle */

		if (td->msg_glyphs[j].g_u16 != 0x20) {
			vg_err = layer_font_glyph_cache_find(font, td->msg_glyphs[j].g_u16, &glyph,
							     text_data_size);
			if (vg_err != VG_LITE_SUCCESS) {
				printk("ERROR: %s failed (%d)\n", __func__, vg_err);
				return vg_err;
			}
			/* Render a glyph as polyline shape */
			vg_err = vg_lite_draw(rt, &glyph->handle, VG_LITE_FILL_EVEN_ODD, &tmatrix,
					      VG_LITE_BLEND_NONE,
					      ARGB_2_VGLITE_COLOR(td->text_color));
			if (vg_err) {
				printk("Error: vg_lite_draw() returned error %d\r\n", vg_err);
				return vg_err;
			}

			/* Update matrix with adv parameters */
			vg_lite_translate(glyph->hx, 0, &tmatrix);
		} else {
			vg_lite_translate(SPACE_HX, 0, &tmatrix);
		}
	}

	return VG_LITE_SUCCESS;
}

static vg_lite_error_t draw_with_constant_fill(vg_lite_buffer_t *rt, vg_lite_path_t *handle,
						vg_lite_fill_t fill_rule,
						vg_lite_matrix_t *tmatrix, uint32_t color)
{
	vg_lite_error_t error;

	error = vg_lite_draw(rt, handle, fill_rule, tmatrix, VG_LITE_BLEND_NONE,
			     ARGB_2_VGLITE_COLOR(color));
	if (error) {
		printk("Error: vg_lite_draw() returned error %d\r\n", error);
	}
	return error;
}

static vg_lite_error_t draw_with_linear_gradient(vg_lite_buffer_t *rt, vg_lite_path_t *handle,
						  vg_lite_fill_t fill_rule,
						  vg_lite_matrix_t *tmatrix,
						  linearGradient_t *linear_grad)
{
	vg_lite_error_t error;
	gradient_cache_entry_t *cachedGradient = NULL;

	error = gradient_cache_find(linear_grad, eLinearGradientCacheEntry, tmatrix,
				     &cachedGradient);
	if (error != VG_LITE_SUCCESS) {
		return error;
	}

	if (cachedGradient == NULL) {
		printk("Error: Failed to get cached linear gradient. Please increase "
		       "MAX_GRADIENT_CACHE.\r\n");
		return VG_LITE_OUT_OF_MEMORY;
	}

	if (g_chip_id == 0x255) {
		error = vg_lite_draw_gradient(rt, handle, fill_rule, tmatrix,
					       &cachedGradient->grad_data.linear_basic
							.basic_gradient,
					       VG_LITE_BLEND_NONE);
	} else {
		error = vg_lite_draw_linear_gradient(rt, handle, fill_rule, tmatrix,
						      &cachedGradient->grad_data.lg.lGradient, 0,
						      VG_LITE_BLEND_NONE,
						      VG_LITE_FILTER_LINEAR);
	}

	return error;
}

static vg_lite_error_t draw_with_radial_gradient(vg_lite_buffer_t *rt, vg_lite_path_t *handle,
						  vg_lite_fill_t fill_rule,
						  vg_lite_matrix_t *tmatrix,
						  radialGradient_t *radial_grad)
{
	vg_lite_error_t error;
	gradient_cache_entry_t *cachedGradient = NULL;

	error = gradient_cache_find(radial_grad, eRadialGradientCacheEntry, tmatrix,
				     &cachedGradient);
	if (error != VG_LITE_SUCCESS) {
		return error;
	}

	if (cachedGradient == NULL) {
		printk("Error: Failed to get cached radial gradient. Please increase "
		       "MAX_GRADIENT_CACHE.\n");
		return VG_LITE_OUT_OF_MEMORY;
	}

	if (g_chip_id != 0x255) {
		error = vg_lite_draw_radial_gradient(rt, handle, fill_rule, tmatrix,
						      &cachedGradient->grad_data.rg.rGradient, 0,
						      VG_LITE_BLEND_NONE,
						      VG_LITE_FILTER_LINEAR);
	}

	return error;
}

static vg_lite_error_t draw_shape_with_fill_type(vg_lite_buffer_t *rt, vg_lite_path_t *handle,
						  hybridPath_t *hybrid_path,
						  vg_lite_fill_t fill_rule,
						  vg_lite_matrix_t *tmatrix, uint32_t color,
						  linearGradient_t *linear_grad,
						  radialGradient_t *radial_grad)
{
	vg_lite_error_t error;

	if (hybrid_path->fillType == NO_FILL_MODE) {
		return VG_LITE_SUCCESS;
	}

	error = vg_lite_set_path_type(handle, hybrid_path->pathType);
	if (error != VG_LITE_SUCCESS) {
		printk("\r\nERROR: Invalid path type!\r\n\r\n");
		return error;
	}

	switch (hybrid_path->fillType) {
	case STROKE:
	case FILL_CONSTANT:
		return draw_with_constant_fill(rt, handle, fill_rule, tmatrix, color);

	case FILL_LINEAR_GRAD:
		return draw_with_linear_gradient(rt, handle, fill_rule, tmatrix, linear_grad);

	case FILL_RADIAL_GRAD:
		return draw_with_radial_gradient(rt, handle, fill_rule, tmatrix, radial_grad);

	default:
		return VG_LITE_INVALID_ARGUMENT;
	}
}

static vg_lite_error_t draw_shape_node(vg_lite_buffer_t *rt, UILayers_t *layer,
					vg_lite_matrix_t *transform_matrix, int shape_idx)
{
	vg_lite_error_t error;
	vg_lite_matrix_t tmatrix;
	const int matrix_size_in_float = 9;

	vg_lite_identity(&tmatrix);
	matrix_multiply(&tmatrix, transform_matrix,
			(const vg_lite_matrix_t *)&layer->img_info
				->transform[shape_idx * matrix_size_in_float]);

	for (int k = 0; k < 2; k++) {
		error = draw_shape_with_fill_type(
			rt, &layer->handle[shape_idx], &layer->mode->hybridPath[2 * shape_idx + k],
			layer->mode->fillRule[shape_idx], &tmatrix, layer->color[shape_idx],
			layer->mode->linearGrads[shape_idx], layer->mode->radialGrads[shape_idx]);

		if (error != VG_LITE_SUCCESS) {
			return error;
		}
	}

	return VG_LITE_SUCCESS;
}

static vg_lite_error_t process_drawable_node(vg_lite_buffer_t *rt, UILayers_t *layer,
					      vg_lite_matrix_t *transform_matrix,
					      svg_node_t node_type, int *shape_idx,
					      int *text_idx)
{
	vg_lite_error_t error;

	switch (node_type) {
	case eRectNode:
	case eCircleNode:
	case eEllipseNode:
	case eLineNode:
	case ePolylineNode:
	case ePolygonNode:
	case ePathNode:
		error = draw_shape_node(rt, layer, transform_matrix, *shape_idx);
		(*shape_idx)++;
		return error;

	case eTextNode:
		error = layer_draw_text(rt, layer, transform_matrix, *text_idx);
		(*text_idx)++;
		return error;

	case eImageNode:
		return layer_draw_images(rt, layer, transform_matrix);

	default:
		printk("Error: Unknown node type %d\r\n", node_type);
		return VG_LITE_INVALID_ARGUMENT;
	}
}

int layer_draw(vg_lite_buffer_t *rt, UILayers_t *layer, vg_lite_matrix_t *transform_matrix)
{
	vg_lite_error_t error;
	int shape_idx = 0;
	int text_idx = 0;

	if (rt == NULL || layer == NULL || transform_matrix == NULL) {
		return VG_LITE_INVALID_ARGUMENT;
	}

	error = get_gpu_chip_info();
	if (error == VG_LITE_NOT_SUPPORT) {
		return VG_LITE_NOT_SUPPORT;
	}

	for (int idx = 0; idx < layer->img_info->drawable_count; idx++) {
		svg_node_t node_type = (svg_node_t)layer->img_info->render_sequence[idx];

		error = process_drawable_node(rt, layer, transform_matrix, node_type, &shape_idx,
					       &text_idx);
		if (error != VG_LITE_SUCCESS) {
			return error;
		}
	}

	vg_lite_finish();

	return VG_LITE_SUCCESS;
}


static vg_lite_error_t init_stroke_for_path(vg_lite_path_t *handle, stroke_info_t *stroke_info)
{
	vg_lite_error_t vg_err;

	vg_err = vg_lite_set_stroke(handle, stroke_info->linecap, stroke_info->linejoin,
				     stroke_info->strokeWidth, stroke_info->miterlimit,
				     stroke_info->dashPattern, stroke_info->dashPatternCnt,
				     stroke_info->dashPhase,
				     ARGB_2_VGLITE_COLOR(stroke_info->strokeColor));

	if (vg_err != VG_LITE_SUCCESS) {
		printk("\r\nERROR: %d Failed to initialize graphic artifacts!\r\n\r\n",
		       __LINE__);
		return vg_err;
	}

	vg_err = vg_lite_set_path_type(handle, VG_LITE_DRAW_STROKE_PATH);
	if (vg_err != VG_LITE_SUCCESS) {
		printk("\r\nERROR: Invalid path type!\r\n\r\n");
		return vg_err;
	}

	vg_err = vg_lite_update_stroke(handle);
	if (vg_err != VG_LITE_SUCCESS) {
		printk("\r\nERROR: %d Failed to initialize graphic artifacts!\r\n\r\n",
		       __LINE__);
		return vg_err;
	}

	return VG_LITE_SUCCESS;
}

static vg_lite_error_t init_path_handles(UILayers_t *layer)
{
	vg_lite_error_t vg_err;
	uint32_t data_size;

	for (int i = 0; i < layer->img_info->path_count; i++) {
		path_info_t *path_info = &layer->img_info->paths_info[i];

		data_size = vg_lite_get_path_length(path_info->path_cmds, path_info->path_length,
						    VG_LITE_S32);

		vg_err = vg_lite_init_path(&layer->handle[i], layer->img_info->data_format,
					   VG_LITE_MEDIUM, data_size, NULL, 0, 0, 0, 0);
		if (vg_err != VG_LITE_SUCCESS) {
			printk("\r\nERROR: Failed to initialize graphic artifacts!\r\n\r\n");
			return vg_err;
		}

		vg_err = vg_lite_append_path(&layer->handle[i], path_info->path_cmds,
					     path_info->path_args, path_info->path_length);
		if (vg_err != VG_LITE_SUCCESS) {
			printk("\r\nERROR: vg_lite_append_path failed! Unable to append path "
			       "data.\r\n\r\n");
			return vg_err;
		}

		layer->handle[i].add_end = path_info->end_path_flag;

		for (int k = 0; k < 2; k++) {
			if (layer->mode->hybridPath[2 * i + k].pathType ==
				    VG_LITE_DRAW_STROKE_PATH ||
			    layer->mode->hybridPath[2 * i + k].pathType ==
				    VG_LITE_DRAW_FILL_STROKE_PATH) {
				stroke_info_t *stroke_info = &layer->img_info->stroke_info[i];

				vg_err = init_stroke_for_path(&layer->handle[i], stroke_info);
				if (vg_err != VG_LITE_SUCCESS) {
					return vg_err;
				}
			}
		}
	}

	return VG_LITE_SUCCESS;
}

int layer_init(UILayers_t *layer)
{
	vg_lite_error_t vg_err;

	if (layer == NULL) {
		return VG_LITE_INVALID_ARGUMENT;
	}

	layer->handle = (vg_lite_path_t *)vg_lite_os_malloc(layer->img_info->path_count *
							    sizeof(vg_lite_path_t));
	if (layer->handle == NULL) {
		printk("\r\nERROR: Memory allocation failed for path!\r\n\r\n");
		return VG_LITE_OUT_OF_MEMORY;
	}
	memset(layer->handle, 0, layer->img_info->path_count * sizeof(vg_lite_path_t));

	layer->matrix = (vg_lite_matrix_t *)vg_lite_os_malloc(layer->img_info->path_count *
							      sizeof(vg_lite_matrix_t));
	if (layer->matrix == NULL) {
		printk("\r\nERROR: Memory allocation failed for matrix!\r\n\r\n");
		return VG_LITE_OUT_OF_MEMORY;
	}

	vg_err = init_path_handles(layer);
	if (vg_err != VG_LITE_SUCCESS) {
		return vg_err;
	}

	vg_err = layer_create_image_buffers(layer);
	if (vg_err != VG_LITE_SUCCESS) {
		return vg_err;
	}

	vg_err = layer_font_glyph_cache_init();
	if (vg_err != VG_LITE_SUCCESS) {
		return vg_err;
	}

	return VG_LITE_SUCCESS;
}


vg_lite_error_t layer_free(UILayers_t *layer)
{
	if (layer == NULL) {
		return VG_LITE_INVALID_ARGUMENT;
	}

	if (layer->handle) {
		vg_lite_os_free(layer->handle);
		layer->handle = NULL;
	}
	if (layer->matrix) {
		vg_lite_os_free(layer->matrix);
		layer->matrix = NULL;
	}

	layer_free_images(layer);

	layer_font_glyph_cache_free();

	return VG_LITE_SUCCESS;
}

/*
 * Copyright 2019, 2023-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS kernel includes. */
#include <math.h>
#include "vg_lite.h"
#include "draw_cube.h"
#include "draw_clock.h"
#include "path_nxplogo.h"
#include "path_tiger.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define RAD(d)                       (d * 3.1415926 / 180.0)
#define TEXTURE_IMAGE_WIDTH          256
#define TEXTURE_IMAGE_HEIGHT         236
#define TEXTURE_IMAGE_BYTE_PER_PIXEL 4
#define TEXTURE_IMAGE_STRIDE         (TEXTURE_IMAGE_WIDTH * TEXTURE_IMAGE_BYTE_PER_PIXEL)
#define TEXTURE_IMAGE_BUFFER_SIZE    (TEXTURE_IMAGE_WIDTH * TEXTURE_IMAGE_HEIGHT)
#define DEMO_PANEL_WIDTH             720
#define DEMO_PANEL_HEIGHT            1280

typedef struct VertexRec {
	float x;
	float y;
	float z;
} vertex_t;

typedef struct NormalRec {
	float x;
	float y;
	float z;
} normal_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

static vg_lite_buffer_t image;

static vertex_t cube_v0 = {-1.0, -1.0, -1.0};
static vertex_t cube_v1 = {1.0, -1.0, -1.0};
static vertex_t cube_v2 = {1.0, 1.0, -1.0};
static vertex_t cube_v3 = {-1.0, 1.0, -1.0};
static vertex_t cube_v4 = {-1.0, -1.0, 1.0};
static vertex_t cube_v5 = {1.0, -1.0, 1.0};
static vertex_t cube_v6 = {1.0, 1.0, 1.0};
static vertex_t cube_v7 = {-1.0, 1.0, 1.0};

static normal_t normal0321 = {0.0, 0.0, -1.0};
static normal_t normal4567 = {0.0, 0.0, 1.0};
static normal_t normal1265 = {1.0, 0.0, 0.0};
static normal_t normal0473 = {-1.0, 0.0, 0.0};
static normal_t normal2376 = {0.0, 1.0, 0.0};
static normal_t normal0154 = {0.0, -1.0, 0.0};

static vg_lite_filter_t filter;
static vg_lite_matrix_t matrix, rotate_3D;
static vertex_t rv0, rv1, rv2, rv3, rv4, rv5, rv6, rv7;
static float nz0321, nz4567, nz5126, nz0473, nz7623, nz0154;
static float cbsize, xoff, yoff, xrot, yrot, zrot, rotstep;
static float rot0, rot1, rot2, rot3, rot4, rot5;
static int rotxyz;

static int fb_width = DEMO_PANEL_WIDTH;
static int fb_height = DEMO_PANEL_HEIGHT;

/* Structure to hold path drawing transformation parameters */
typedef struct {
	vg_lite_float_t translate_x;
	vg_lite_float_t translate_y;
	vg_lite_float_t scale_x;
	vg_lite_float_t scale_y;
	vg_lite_float_t rotate_deg;
} path_transform_t;

/* Structure to hold path drawing data */
typedef struct {
	vg_lite_path_t *paths;
	uint32_t *colors;
	int count;
} path_data_t;

/*******************************************************************************
 * Code
 ******************************************************************************/

void scale_cube(vertex_t *vertex, float scale)
{
	/* Scale cube vertex coordinates to proper size */
	vertex->x *= scale;
	vertex->y *= scale;
	vertex->z *= scale;
}

void compute_rotate(float rx, float ry, float rz, vg_lite_matrix_t *rotate)
{
	/*
	 * Rotation angles rx, ry, rz (degree) for axis X, Y, Z
	 * Compute 3D rotation matrix base on rotation angle rx, ry, rz about axis X, Y, Z.
	 */

	static float rx_l = -1, ry_l = -1, rz_l = -1;

	static float rx_sin, rx_cos, ry_sin, ry_cos, rz_sin, rz_cos;

	if (rx_l != rx) {
		rx_sin = sin(RAD((double)rx));
		rx_cos = cos(RAD((double)rx));
	}

	if (ry_l != ry) {
		ry_sin = sin(RAD((double)ry));
		ry_cos = cos(RAD((double)ry));
	}

	if (rz_l != rz) {
		rz_sin = sin(RAD((double)rz));
		rz_cos = cos(RAD((double)rz));
	}

	rotate->m[0][0] = rz_cos * ry_cos;
	rotate->m[0][1] = rz_cos * ry_sin * rx_sin - rz_sin * rx_cos;
	rotate->m[0][2] = rz_cos * ry_sin * rx_cos + rz_sin * rx_sin;
	rotate->m[1][0] = rz_sin * ry_cos;
	rotate->m[1][1] = rz_sin * ry_sin * rx_sin + rz_cos * rx_cos;
	rotate->m[1][2] = rz_sin * ry_sin * rx_cos - rz_cos * rx_sin;
	rotate->m[2][0] = -ry_sin;
	rotate->m[2][1] = ry_cos * rx_sin;
	rotate->m[2][2] = ry_cos * rx_cos;
}

void transform_rotate(vg_lite_matrix_t *rotate, vertex_t *vertex, vertex_t *rc, float tx, float ty)
{
	/* Compute the new cube vertex coordinates transformed by the rotation matrix */
	rc->x = rotate->m[0][0] * vertex->x + rotate->m[0][1] * vertex->y +
		rotate->m[0][2] * vertex->z;
	rc->y = rotate->m[1][0] * vertex->x + rotate->m[1][1] * vertex->y +
		rotate->m[1][2] * vertex->z;
	rc->z = rotate->m[2][0] * vertex->x + rotate->m[2][1] * vertex->y +
		rotate->m[2][2] * vertex->z;

	/* Translate the vertex in XY plane */
	rc->x += tx;
	rc->y += ty;
}

void transform_normalZ(vg_lite_matrix_t *rotate, normal_t *nVec, float *nZ)
{
	/* Compute the new normal Z coordinate transformed by the rotation matrix */
	*nZ = rotate->m[2][0] * nVec->x + rotate->m[2][1] * nVec->y + rotate->m[2][2] * nVec->z;
}

/* Calculate the homogeneous matrix to map a rectangle image (0,0),(w,0),(w,h),(0,h)
 *  to a parallelogram (x0,y0),(x1,y1),(x2,y2),(x3,y3). An affine transformation maps
 *  a point (x, y) into the point(x*sx + y*shx + tx, x*shy + y*sy + ty) using homogeneous
 *  matrix multiplication. So having the following equations:
 *     x0 = tx;
 *     y0 = ty;
 *     x1 = w*sx + tx;
 *     y1 = w*shy + ty;
 *     x3 = h*shx + tx;
 *     y3 = h*sy + ty;
 */
void transform_blit(float w, float h, vertex_t *v0, vertex_t *v1, vertex_t *v2, vertex_t *v3,
		    vg_lite_matrix_t *matrix)
{
	float sx, sy, shx, shy, tx, ty;

	/*
	 * Compute 3x3 image transform matrix to map a rectangle image (w,h) to
	 * a parallelogram (x0,y0), (x1,y1), (x2,y2), (x3,y3) counterclock wise.
	 */
	sx = (v1->x - v0->x) / w;
	sy = (v3->y - v0->y) / h;
	shx = (v3->x - v0->x) / h;
	shy = (v1->y - v0->y) / w;
	tx = v0->x;
	ty = v0->y;

	/* Set the blit transformation matrix */
	matrix->m[0][0] = sx;
	matrix->m[0][1] = shx;
	matrix->m[0][2] = tx;
	matrix->m[1][0] = shy;
	matrix->m[1][1] = sy;
	matrix->m[1][2] = ty;
	matrix->m[2][0] = 0.0;
	matrix->m[2][1] = 0.0;
	matrix->m[2][2] = 1.0;
}

static int allocate_image_buffer(vg_lite_buffer_t *buffer)
{
	vg_lite_error_t error = VG_LITE_SUCCESS;

	buffer->width = TEXTURE_IMAGE_WIDTH;
	buffer->height = TEXTURE_IMAGE_HEIGHT;
	buffer->format = VG_LITE_BGRA8888;
	buffer->tiled = VG_LITE_TILED;

	error = vg_lite_allocate(buffer);
	if (error != VG_LITE_SUCCESS) {
		printk("Could not allocate buffer\n\r");
		return -1;
	}

	return 1;
}

static int draw_path(vg_lite_buffer_t *buffer, vg_lite_color_t back_color,
		     const path_data_t *path_data, const path_transform_t *transform)
{
	vg_lite_error_t error = VG_LITE_SUCCESS;
	vg_lite_matrix_t matrix;

	/* Draw the path using the matrix. */
	vg_lite_identity(&matrix);
	vg_lite_translate(buffer->width / 2.0, buffer->height / 2.0, &matrix);
	vg_lite_rotate(transform->rotate_deg, &matrix);
	vg_lite_translate(-buffer->width / 2.0, -buffer->height / 2.0, &matrix);
	vg_lite_translate(transform->translate_x, transform->translate_y, &matrix);
	vg_lite_scale(transform->scale_x, transform->scale_y, &matrix);
	vg_lite_clear(buffer, NULL, back_color);

	for (uint8_t count = 0; count < path_data->count; count++) {
		error = vg_lite_draw(buffer, &path_data->paths[count], VG_LITE_FILL_EVEN_ODD,
				     &matrix, VG_LITE_BLEND_NONE, path_data->colors[count]);
		if (error) {
			printk("vg_lite_draw() returned error %d\r\n", error);
			for (uint8_t i = 0; i < count; i++) {
				vg_lite_clear_path(&path_data->paths[i]);
			}
			vg_lite_close();
			return -1;
		}
	}

	return 1;
}

bool load_texture_images(void)
{
	/* Set image filter type. */
	filter = VG_LITE_FILTER_BI_LINEAR;

	/* Load the image */
	if (allocate_image_buffer(&image) != 1) {
		printk("Allocate buffer error\n");
		return false;
	}

	/* Scale the cube to proper size */
	cbsize = fb_width / 4.0;
	scale_cube(&cube_v0, cbsize);
	scale_cube(&cube_v1, cbsize);
	scale_cube(&cube_v2, cbsize);
	scale_cube(&cube_v3, cbsize);
	scale_cube(&cube_v4, cbsize);
	scale_cube(&cube_v5, cbsize);
	scale_cube(&cube_v6, cbsize);
	scale_cube(&cube_v7, cbsize);

	/* Translate the cube to the center of framebuffer */
	xoff = fb_width / 2.0;
	yoff = fb_height / 2.0;

	/* Set the initial cube rotation degree and step */
	xrot = 10.0;
	yrot = 20.0;
	zrot = 30.0;
	rotstep = 1.0;
	rotxyz = 0;

	return true;
}

void draw_cube(vg_lite_buffer_t *rt)
{
	rot0 += 1.0;
	rot1 += 2.0;
	rot2 += 3.0;
	rot3 += 4.0;
	rot4 += 5.0;
	rot5 += 6.0;
	if (rot0 > 360.0f) {
		rot0 -= 360.0;
	}
	if (rot1 > 360.0f) {
		rot1 -= 360.0;
	}
	if (rot2 > 360.0f) {
		rot2 -= 360.0;
	}
	if (rot3 > 360.0f) {
		rot3 -= 360.0;
	}
	if (rot4 > 360.0f) {
		rot4 -= 360.0;
	}
	if (rot5 > 360.0f) {
		rot5 -= 360.0;
	}
	/* Rotation angles (degree) for axis X, Y, Z */
	compute_rotate(xrot, yrot, zrot, &rotate_3D);
	if (rotxyz == 0) {
		xrot += rotstep;
		if (xrot > 380.0f) {
			xrot = 20;
			rotxyz = 1;
		}
	} else if (rotxyz == 1) {
		yrot += rotstep;
		if (yrot > 400.0f) {
			yrot = 40;
			rotxyz = 0;
		}
	} else {
		/* Defensive: reset to known state if rotxyz has unexpected value */
		rotxyz = 0;
	}

	/* Compute the new cube vertex coordinates transformed by the rotation matrix */
	transform_rotate(&rotate_3D, &cube_v0, &rv0, xoff, yoff);
	transform_rotate(&rotate_3D, &cube_v1, &rv1, xoff, yoff);
	transform_rotate(&rotate_3D, &cube_v2, &rv2, xoff, yoff);
	transform_rotate(&rotate_3D, &cube_v3, &rv3, xoff, yoff);
	transform_rotate(&rotate_3D, &cube_v4, &rv4, xoff, yoff);
	transform_rotate(&rotate_3D, &cube_v5, &rv5, xoff, yoff);
	transform_rotate(&rotate_3D, &cube_v6, &rv6, xoff, yoff);
	transform_rotate(&rotate_3D, &cube_v7, &rv7, xoff, yoff);

	/* Compute the surface normal direction to determine the front/back face */
	transform_normalZ(&rotate_3D, &normal0321, &nz0321);
	transform_normalZ(&rotate_3D, &normal4567, &nz4567);
	transform_normalZ(&rotate_3D, &normal1265, &nz5126);
	transform_normalZ(&rotate_3D, &normal0473, &nz0473);
	transform_normalZ(&rotate_3D, &normal2376, &nz7623);
	transform_normalZ(&rotate_3D, &normal0154, &nz0154);

	path_data_t tiger_data = {tiger_path, tiger_color_data, tiger_path_count};
	path_data_t nxp_data = {nxp_path, nxp_color_data, nxp_path_count};
	path_transform_t transform;

	if (nz0321 > 0.0f) {
		transform = (path_transform_t){90.0f, 90.0f, 1.5f, 1.5f, rot0};
		draw_path(&image, 0xFFFF0000U, &tiger_data, &transform);

		/*
		 * Compute 3x3 image transform matrix to map a rectangle image (w,h) to
		 * a parallelogram (x0,y0), (x1,y1), (x2,y2), (x3,y3) counterclock wise.
		 */
		transform_blit(image.width, image.height, &rv0, &rv3, &rv2, &rv1, &matrix);
		/* Blit the image using the matrix */
		vg_lite_blit(rt, &image, &matrix, VG_LITE_BLEND_SCREEN, 0, filter);
	}

	if (nz4567 > 0.0f) {
		/* Draw NXP logo on the image1 */
		transform = (path_transform_t){25.0f, 80.5f, 3.0f, 3.0f, 0.0f};
		if (draw_path(&image, 0xFFFFFFFFU, &nxp_data, &transform) != 1) {
			printk("load image1 file error\n");
		}
		/*
		 * Compute 3x3 image transform matrix to map a rectangle image (w,h) to
		 * a parallelogram (x0,y0), (x1,y1), (x2,y2), (x3,y3) counterclock wise.
		 */
		transform_blit(image.width, image.height, &rv4, &rv5, &rv6, &rv7, &matrix);

		/* Blit the image using the matrix */
		vg_lite_blit(rt, &image, &matrix, VG_LITE_BLEND_SCREEN, 0, filter);
	}

	if (nz5126 > 0.0f) {
		transform = (path_transform_t){90.0f, 90.0f, 1.5f, 1.5f, rot2};
		draw_path(&image, 0xFFE7BFC8U, &tiger_data, &transform);
		/*
		 * Compute 3x3 image transform matrix to map a rectangle image (w,h) to
		 * a parallelogram (x0,y0), (x1,y1), (x2,y2), (x3,y3) counterclock wise.
		 */
		transform_blit(image.width, image.height, &rv5, &rv1, &rv2, &rv6, &matrix);

		/* Blit the image using the matrix */
		vg_lite_blit(rt, &image, &matrix, VG_LITE_BLEND_SCREEN, 0, filter);
	}

	if (nz0473 > 0.0f) {
		/* draw the clock */
		draw_clock(&image, 0xFF00EE00U);
		/*
		 * Compute 3x3 image transform matrix to map a rectangle image (w,h) to
		 * a parallelogram (x0,y0), (x1,y1), (x2,y2), (x3,y3) counterclock wise.
		 */
		transform_blit(image.width, image.height, &rv0, &rv4, &rv7, &rv3, &matrix);

		/* Blit the image using the matrix */
		vg_lite_blit(rt, &image, &matrix, VG_LITE_BLEND_SCREEN, 0, filter);
	}

	if (nz7623 > 0.0f) {
		transform = (path_transform_t){90.0f, 90.0f, 1.5f, 1.5f, rot4};
		draw_path(&image, 0xFFEAD999U, &tiger_data, &transform);
		/*
		 * Compute 3x3 image transform matrix to map a rectangle image (w,h) to
		 * a parallelogram (x0,y0), (x1,y1), (x2,y2), (x3,y3) counterclock wise.
		 */
		transform_blit(image.width, image.height, &rv7, &rv6, &rv2, &rv3, &matrix);

		/* Blit the image using the matrix */
		vg_lite_blit(rt, &image, &matrix, VG_LITE_BLEND_SCREEN, 0, filter);
	}

	if (nz0154 > 0.0f) {
		/* draw the clock */
		draw_clock(&image, 0xFF0BE4EFU);

		/*
		 * Compute 3x3 image transform matrix to map a rectangle image (w,h) to
		 * a parallelogram (x0,y0), (x1,y1), (x2,y2), (x3,y3) counterclock wise.
		 */
		transform_blit(image.width, image.height, &rv0, &rv1, &rv5, &rv4, &matrix);

		/* Blit the image using the matrix */
		vg_lite_blit(rt, &image, &matrix, VG_LITE_BLEND_SCREEN, 0, filter);
	}
}

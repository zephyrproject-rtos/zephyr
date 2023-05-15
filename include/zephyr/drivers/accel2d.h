/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for GPU 2D accelerators
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ACCEL2D_H_
#define ZEPHYR_INCLUDE_DRIVERS_ACCEL2D_H_

#include <zephyr/drivers/display.h>

/**
 * @brief ACCEL2D Driver APIs
 * @defgroup accel2d_interface GPU 2D interface
 * @ingroup io_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 2D accelerator source data type
 *
 * Type of source data provided to 2D accelerator
 */
enum accel2d_src_type {
	/** Solid color (ARGB8888 format)*/
	ACCEL2D_SRC_COLOR,
	/** Gradient definition used as source */
	ACCEL2D_SRC_GRADIENT,
};

/**
 * @brief 2D accelerator coordinate data
 *
 * 2D accelerator coordinate data. Given on a 2D plane, starting at upper left
 * corner of display
 */
struct accel2d_coords {
	/** X coordinate */
	uint32_t x;
	/** Y coordinate */
	uint32_t y;
};

/**
 * @brief 2D accelerator buffer descriptor
 *
 * Describes buffer for 2D accelerator. Note that some GPUs may have alignment
 * restrictions on the stride or location of this buffer.
 */
struct accel_buf {
	/** Pixel data buffer */
	uint8_t *buf;
	/** Pixel buffer format */
	enum display_pixel_format fmt;
	/** Buffer descriptor */
	struct display_buffer_descriptor desc;
};

/**
 * @brief 2D accelerator gradient direction
 *
 * 2D accelerator gradient direction. Gradient can be horizontal or vertical
 */
enum accel2d_grad_dir {
	/** Gradient in horizontal direction, from left to right */
	ACCEL2D_GRAD_DIR_HOR,
	/** Gradient in vertical direction, from top to bottom */
	ACCEL2D_GRAD_DIR_VER,
};

/**
 * @brief 2D accelerator gradient descriptor
 *
 * describes gradient source properties
 */
struct accel2d_gradient {
	/** ARGB888 formatted colors to use in gradient */
	uint32_t *colors;
	/** Fractional position of each color, 0-255 */
	uint8_t *frac;
	/** Count of colors in gradient */
	uint8_t color_count;
	/** Gradient direction */
	enum accel2d_grad_dir direction;
};

union accel2d_src_data {
	/** Color, ARGB8888 format */
	uint32_t color;
	/** Gradient description */
	struct accel2d_gradient grad;
};

/**
 * @brief 2D accelerator source data descriptor
 *
 * Describes source data for 2D accelerator. This can be a pixel color or
 * gradient.
 */
struct accel2d_src {
	/** Source data type */
	enum accel2d_src_type type;
	/** Source data for 2D accelerator */
	union accel2d_src_data data;
};

/**
 * @brief 2D accelerator rectangle descriptor
 *
 * Description of rectangle for 2D accelerator to draw. This can be used with
 * the @ref accel2d_draw_rect function to render a rectangle
 */
struct accel2d_rect {
	/** Upper left coordinate */
	struct accel2d_coords upper_left;
	/** Upper right coordinate */
	struct accel2d_coords upper_right;
	/** Lower left coordinate */
	struct accel2d_coords lower_left;
	/** Lower right coordinate */
	struct accel2d_coords lower_right;
};

/**
 * @brief 2D accelerator arc descriptor
 *
 * Description of arc for the 2D accelerator to draw. This can be used with the
 * @ref accel2d_draw_arc to create an arc.
 */
struct accel2d_arc {
	/** Center point of arc */
	struct accel2d_coords center;
	/** Inner radius of arc, in pixels */
	uint32_t inner_radius;
	/** Outer radius of arc, in pixels */
	uint32_t outer_radius;
	/** Starting angle, in degrees */
	uint32_t start_angle;
	/** End angle, in degrees */
	uint32_t end_angle;
};

/**
 * @brief 2D accelerator blend method
 *
 * Sets 2D accelerator blending type. These types are based off the Porter-Duff
 * blend modes. For these modes, the resulting pixels are defined as follows:
 * [alpha, color]. These results can be made of the following components:
 * Sa- source alpha, Da- destination alpha, Sc- source color, Dc- destination
 * color.
 */
enum accel2d_blend {
	/** Clear output buffer: [0, 0] */
	ACCEL2D_BLEND_CLEAR = BIT(0),
	/** Replace output buffer with source: [Sc, Sc] */
	ACCEL2D_BLEND_SRC = BIT(1),
	/** Do not use source buffer, keep destination buffer as is: [Dc, Dc] */
	ACCEL2D_BLEND_DST = BIT(2),
	/** Place source buffer over destination buffer:
	 * [Sa + Da * (1 - Sa), Sc + Dc * (1 - Sa)]
	 */
	ACCEL2D_BLEND_SRC_OVER = BIT(3),
	/** Place destination buffer over source buffer:
	 * [Da + Sa * (1 - Da), Dc + Sc * (1 - Da)]
	 */
	ACCEL2D_BLEND_DST_OVER = BIT(3),
	/** Render source at intersection of destination and source:
	 * [Sa * Da, Sc * Da]
	 */
	ACCEL2D_BLEND_SRC_IN = BIT(4),
	/** Render source anywhere it does not intersect with destination:
	 * [Sa * (1 - Da), Sc * (1 - Da)]
	 */
	ACCEL2D_BLEND_SRC_OUT = BIT(5),
	/** Render destination at intersection of destination and source:
	 * [Sa * Da, Dc * Sa]
	 */
	ACCEL2D_BLEND_DST_IN = BIT(6),
	/** Render destination anywhere is does not intersect with source:
	 * [Da * (1 - Sa), Dc * (1 - Sa) ]
	 */
	ACCEL2D_BLEND_DST_OUT = BIT(7),
	/** Render destination, then render source atop it anywhere they
	 * intersect: [Da, Sc * Da + Dc * (1 - Sa)]
	 */
	ACCEL2D_BLEND_SRC_ATOP = BIT(8),
	/** Render source, then render destination atop it anywhere they
	 * intersect: [Sa, Dc * Sa + Sc * (1 - Da)]
	 */
	ACCEL2D_BLEND_DST_ATOP = BIT(9),
	/** Render source and destination anywhere they don't intersect:
	 * [Sa + Da - 2 * Sa * Da, Sc * (1 - Da) + Dc * (1 - Sa)]
	 */
	ACCEL2D_BLEND_XOR = BIT(10),
	/** Combine source and destination, adding pixels together at overlap:
	 * [Sa + Da, Sc + Dc]
	 */
	ACCEL2D_BLEND_PLUS = BIT(11),
	/** Modulate source and destination, multiplying pixels at overlap:
	 * [Sa * Da, Sc * Dc]
	 */
	ACCEL2D_BLEND_MODULATE = BIT(12),
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */

/* Helper macro to define 2D accelerator opcode enums */
#define ACCEL2D_OPCODE_DEF(num_args, id) (((num_args) << 4) | ((id)&0xF))

/**
 * @brief 2D accelerator PATH opcodes
 *
 * 2D accelerator path opcodes. These OPCodes describe how to move the 2D
 * accelerator drawing cursor, and how to draw paths between points. The opcodes
 * currently support the following general operations:
 * - moving cursor
 * - drawing line
 * - drawing quadratic bezier curve
 * - drawing cubic bezier curve
 * - drawing clockwise and counterclockwise arc
 */
enum accel2d_opcodes {
	ACCEL2D_OPCODE_MOVE = ACCEL2D_OPCODE_DEF(2, 0),
	ACCEL2D_OPCODE_MOVE_REL = ACCEL2D_OPCODE_DEF(2, 1),
	ACCEL2D_OPCODE_LINE = ACCEL2D_OPCODE_DEF(2, 3),
	ACCEL2D_OPCODE_LINE_REL = ACCEL2D_OPCODE_DEF(2, 4),
	ACCEL2D_OPCODE_QUAD = ACCEL2D_OPCODE_DEF(4, 5),
	ACCEL2D_OPCODE_QUAD_REL = ACCEL2D_OPCODE_DEF(4, 6),
	ACCEL2D_OPCODE_CUBIC = ACCEL2D_OPCODE_DEF(6, 7),
	ACCEL2D_OPCODE_CUBIC_REL = ACCEL2D_OPCODE_DEF(6, 8),
	ACCEL2D_OPCODE_ARC_CLOCKWISE = ACCEL2D_OPCODE_DEF(5, 9),
	ACCEL2D_OPCODE_ARC_CLOCKWISE_REL = ACCEL2D_OPCODE_DEF(5, 10),
	ACCEL2D_OPCODE_ARC_COUNTER_CW = ACCEL2D_OPCODE_DEF(5, 11),
	ACCEL2D_OPCODE_ARC_COUNTER_CW_REL = ACCEL2D_OPCODE_DEF(5, 12),
	ACCEL2D_OPCODE_CLOSE_PATH = ACCEL2D_OPCODE_DEF(0, 13),
};

/* Generic opcode helper- not intended for users */
#define ACCEL2D_OP_GENERIC(op, rel, ...) ((rel ? op : op##_REL), __VA_ARGS__)

/** @endcond */

/**
 * @brief 2D accelerator MOVE opcode
 *
 * Absolute mode: Moves cursor to (x, y)
 * Relative mode: Moves cursor to (start_x + x, start_y + y)
 * @param rel: should the coordinates be relative to the active coordinate?
 * @param x: x coordinate for cursor, in pixels
 * @param y: y coordinate for cursor, in pixels
 */
#define ACCEL2D_OP_MOVE(rel, x, y)                                             \
	ACCEL2D_OP_GENERIC(ACCEL2D_OPCODE_MOVE, rel, x, y)

/**
 * @brief 2D accelerator LINE opcode
 *
 * Absolute mode: Draws line from (start_x, start_y) to (x, y)
 * Relative mode: from (start_x, start_y) to (start_x + x, start_y + y)
 * @param rel: should the coordinates be relative to the active coordinate?
 * @param x: x coordinate to draw line to, in pixels
 * @param y: y coordinate to draw line to, in pixels
 */
#define ACCEL2D_OP_LINE(rel, x, y)                                             \
	ACCEL2D_OP_GENERIC(ACCEL2D_OPCODE_LINE, rel, x, y)

/**
 * @brief 2D accelerator QUAD opcode
 *
 * Draws Quadratic bezier curve (one control point)
 * Absolute mode: from (start_x, start_y) to (x, y) with control point (cx, cy)
 * Relative mode: from (start_x, start_y) to (start_x + x, start_y + y) with
 * control point (start_x + cx, start_y + cy)
 * @param rel: should the coordinates be relative to the active coordinate?
 * @param cx: x coordinate of control point, in pixels
 * @param cy: y coordinate of control point, in pixels
 * @param x: x coordinate to draw curve to, in pixels
 * @param y: y coordinate to draw curve to, in pixels
 */
#define ACCEL2D_OP_QUAD(rel, cx, cy, x, y)                                     \
	ACCEL2D_OP_GENERIC(ACCEL2D_OPCODE_QUAD, rel, cx, cy, x, y)

/**
 * @brief 2D accelerator CUBIC opcode
 *
 * Draws Cubic bezier curve (two control points)
 * Absolute mode: from (start_x, start_y) to (x, y) with control point (cx1,
 * cy1) and control point (cx2, cx2) Relative mode: from (start_x, start_y) to
 * (start_x + x, start_y + y) with control point (start_x + cx1, start_y + cy1)
 * and (start_x + cx2, start_y + cy2)
 * @param rel: should the coordinates be relative to the active coordinate?
 * @param cx1: x coordinate of first control point, in pixels
 * @param cy1: y coordinate of first control point, in pixels
 * @param cx2: x coordinate of second control point, in pixels
 * @param cy2: y coordinate of second control point, in pixels
 * @param x: x coordinate to draw curve to, in pixels
 * @param y: y coordinate to draw curve to, in pixels
 */
#define ACCEL2D_OP_CUBIC(rel, cx1, cy1, cx2, cy2, x, y)                        \
	ACCEL2D_OP_GENERIC(ACCEL2D_OPCODE_CUBIC, rel, cx1, cy1, cx2, cy2, x, y)

/**
 * @brief 2D accelerator clockwise arc opcode
 *
 * Draws clockwise arc
 * Absolute mode: from (start_x, start_y) to (x, y) with rotation rot,
 * horizontal radius rh, and vertical radius rv
 * Relative mode: from (start_x, start_y) to (start_x + x, start_y + y)
 * with rotation rot, horizontal radius rh, and vertical radius rv
 * @param rel: should the coordinates be relative to the active coordinate?
 * @param rh: horizontal arc radius, in pixels
 * @param rv: vertical arc radius, in pixels
 * @param rot: rotation angle in radians
 * @param x: x coordinate to draw curve to, in pixels
 * @param y: y coordinate to draw curve to, in pixels
 */
#define ACCEL2D_OP_ARC_CLOCKWISE(rel, rel, rh, rv, rot, x, y)                  \
	ACCEL2D_OP_GENERIC(ACCEL2D_OPCODE_ARC_CLOCKWISE, rel, rh, rv, rot, x, y)

/**
 * @brief 2D accelerator counter clockwise arc opcode
 *
 * Draws counter clockwise arc
 * Absolute mode: from (start_x, start_y) to (x, y) with rotation rot,
 * horizontal radius rh, and vertical radius rv
 * Relative mode: from (start_x, start_y) to (start_x + x, start_y + y)
 * with rotation rot, horizontal radius rh, and vertical radius rv
 * @param rel: should the coordinates be relative to the active coordinate?
 * @param rh: horizontal arc radius, in pixels
 * @param rv: vertical arc radius, in pixels
 * @param rot: rotation angle in radians
 * @param x: x coordinate to draw curve to, in pixels
 * @param y: y coordinate to draw curve to, in pixels
 */
#define ACCEL2D_OP_ARC_COUNTER_CLOCKWISE(rel, rel, rh, rv, rot, x, y)          \
	ACCEL2D_OP_GENERIC(ACCEL2D_OPCODE_ARC_COUNTER_CW, rel, rh, rv, rot, x, \
			   y)

/**
 * @brief 2D accelerator close path opcode
 *
 * Closes a 2D accelerator path. A new path can then be started with the
 * ACCEL2D_MOVE opcode. The current path must have the same start and end point
 * to be closed with this command.
 */
#define ACCEL2D_OP_CLOSE_PATH()                                                \
	ACCEL2D_OP_GENERIC(ACCEL2D_OPCODE_CLOSE_PATH, false)

/**
 * @brief 2D accelerator matrix definition
 *
 * 2D accelerator affine transformation matrix definition. This 3x2 matrix
 * consists of a 2x2 rotation matrix, and a column vector for translation of the
 * image. It can be used with the 2D accelerator path functions to perform
 * arbitrary 2D transformations the 2D accelerator input buffer or output path.
 */
struct accel2d_matrix {
	/** Transformation matrix definition */
	float m[3][2];
};

/**
 * @brief 2D accelerator fill rules for paths
 *
 * Sets the rules for filling arbitrary 2D accelerator paths.
 */
enum accel2d_fill_rule {
	/** Draw any pixel within at least one path */
	ACCEL2D_FILL_ANY,
	/** Draw only pixels that reside within an odd number of paths */
	ACCEL2D_FILL_ODD,
};

/**
 * @brief 2D accelerator image rotation setting
 *
 * Note that the rotation setting here causes the 2D accelerator to treat the
 * output buffer as rotated when copying pixels.
 */
enum accel2d_rotate {
	/** Do not rotate output image */
	ACCEL2D_ROTATE_NONE,
	/** Rotate output image 90 degrees */
	ACCEL2D_ROTATE_90,
	/** Rotate output image 180 degrees */
	ACCEL2D_ROTATE_180,
	/** Rotate output image 270 degrees */
	ACCEL2D_ROTATE_270,
};

/**
 * @brief 2D accelerator scale factor
 *
 * Scales output buffer up or down by a fixed factor
 */
enum accel2d_scale {
	ACCEL2D_SCALE_2,     /**< x2 */
	ACCEL2D_SCALE_1_875, /**< x1.875 */
	ACCEL2D_SCALE_1_75,  /**< x1.75 */
	ACCEL2D_SCALE_1_625, /**< x1.625 */
	ACCEL2D_SCALE_1_50,  /**< x1.50 */
	ACCEL2D_SCALE_1_375, /**< x1.375 */
	ACCEL2D_SCALE_1_25,  /**< x1.25 */
	ACCEL2D_SCALE_1_125, /**< x1.125 */
	ACCEL2D_SCALE_1,     /**< x1 */
	ACCEL2D_SCALE_0_75,  /**< 0.75 */
	ACCEL2D_SCALE_0_50,  /**< 0.5 */
	ACCEL2D_SCALE_0_25,  /**< 0.25 */
	ACCEL2D_SCALE_1_3,   /**< 1/3 */
	ACCEL2D_SCALE_1_4,   /**< 1/4 */
	ACCEL2D_SCALE_1_8,   /**< 1/8 */
};

/**
 * @brief 2D accelerator capabilities flags
 *
 * Capabilities data for a 2D accelerator device.
 */
struct accel2d_capabilities {
	/* Blending support */
	unsigned int blend_support; /**< Bitmask of supported blending modes */
	unsigned short alignment; /**< Alignment requirement for 2D accelerator buffers */
	/* Basic API support */
	unsigned int rect_support: 1; /**< support for accel2d_draw_rect */
	unsigned int arc_support: 1;  /**< support for accel2d_draw_arc */
	unsigned int line_support: 1; /**< support for accel2d_draw_line */
	unsigned int blit_support: 1; /**< support for accel2d_blit_img */
	/* Path API support */
	unsigned int path_support: 1;   /**< support for accel2d path APIs */
	unsigned int direct_support: 1; /**< support for accel2d_write_display */
};

/**
 * @brief Draw a simple rectangle using the 2D accelerator
 *
 * Draws a simple rectangle using the 2D accelerator hardware.
 * @param dev: 2D accelerator device object
 * @param buf: 2D accelerator output buffer description
 * @param src: Input source description.
 * @param rect: rectangle output description
 * @param blend: 2D accelerator porter-duff blend setting
 * @retval 0 command was queued successfully
 * @retval -ENOSYS if this 2D accelerator does not support this feature
 * @retval -ENOTSUP if the 2D accelerator does not support the
 *          requested combination of settings
 * @retval -EINVAL invalid buffer alignment settings for the 2D accelerator
 * @retval -EBUSY @ref accel2d_render must be called before this operation can
 *          proceed, as no more rendering can be queued on the 2D accelerator
 * hardware.
 * @retval -EIO Underlying 2D accelerator encountered an I/O error.
 */
int accel2d_draw_rect(const struct device *dev, struct accel_buf *buf,
		      struct accel2d_src *src, struct accel2d_rect *rect,
		      enum accel2d_blend blend);

/**
 * @brief Draw an arc using the 2D accelerator
 *
 * Draws an arc using the 2D accelerator hardware. Can also draw a full circle.
 * @param dev: 2D accelerator device object
 * @param buf: 2D accelerator output buffer description
 * @param src: Input source description.
 * @param arc: arc output description
 * @param blend: 2D accelerator porter-duff blend setting
 * @retval 0 command was queued successfully
 * @retval -ENOSYS if this 2D accelerator does not support this feature
 * @retval -ENOTSUP if the 2D accelerator does not support the
 *          requested combination of settings
 * @retval -EINVAL invalid buffer alignment settings for the 2D accelerator
 * @retval -EBUSY @ref accel2d_render must be called before this operation can
 *          proceed, as no more rendering can be queued on the 2D accelerator
 * hardware.
 * @retval -EIO Underlying 2D accelerator encountered an I/O error.
 */
int accel2d_draw_arc(const struct device *dev, struct accel_buf *buf,
		     struct accel2d_src *src, struct accel2d_arc *arc,
		     enum accel2d_blend blend);

/**
 * @brief Draw a line using the 2D accelerator
 *
 * Draws a line using the 2D accelerator hardware.
 * @param dev: 2D accelerator device object
 * @param buf: 2D accelerator output buffer description
 * @param src: input source description
 * @param start: start coordinates
 * @param end: end coordinates
 * @param width: width of line
 * @param blend: 2D accelerator porter-duff blend setting
 * @retval 0 command was queued successfully
 * @retval -ENOSYS if this 2D accelerator does not support this feature
 * @retval -ENOTSUP if the 2D accelerator does not support the
 *          requested combination of settings
 * @retval -EINVAL invalid buffer alignment settings for the 2D accelerator
 * supporting this operation
 * @retval -EBUSY @ref accel2d_render must be called before this operation can
 *          proceed, as no more rendering can be queued on the 2D accelerator
 * hardware.
 * @retval -EIO Underlying 2D accelerator encountered an I/O error.
 */
int accel2d_draw_line(const struct device *dev, struct accel_buf *buf,
		      struct accel2d_src *src, struct accel2d_coords *start,
		      struct accel2d_coords *end, uint32_t width,
		      enum accel2d_blend blend);

/**
 * @brief Draw an arbitrary gpu path
 *
 * This function leverages the 2D accelerator tesselation matrix and path opcode
 * definitions to enable the user to draw an arbitrary shape using the
 * 2D accelerator. The 2D accelerator path commands must form closed paths.
 * Multiple paths can be drawn in one command by using the ACCEL2D_CLOSE_PATH
 * opcode, followed by the ACCEL2D_MOVE opcode in the path definition to start a
 * new path.
 * @param dev: 2D accelerator device object
 * @param buf: 2D accelerator output buffer description
 * @param src: input source description
 * @param path: uint32_t array of ACCEL2D_OP opcode definitions.
 * @param path_len: length of path array, in bytes
 * @param fill_rule: fill rule for path.
 * @param transform: gpu affine transformation matrix.
 * @param blend: 2D accelerator porter-duff blend setting
 * @retval 0 command was queued successfully
 * @retval -ENOSYS if this 2D accelerator does not support this feature
 * @retval -ENOTSUP if the 2D accelerator does not support the
 *          requested combination of settings
 * @retval -EINVAL invalid buffer alignment settings for the 2D accelerator
 * @retval -EBUSY @ref accel2d_render must be called before this operation can
 *          proceed, as no more rendering can be queued on the 2D accelerator
 * hardware.
 * @retval -EIO Underlying 2D accelerator encountered an I/O error.
 */
int accel2d_draw_path(const struct device *dev, struct accel_buf *buf,
		      struct gpu_src *src, uint32_t *path, uint32_t path_len,
		      enum accel2d_fill_rule fill_rule,
		      struct accel2d_matrix *transform,
		      enum accel2d_blend blend);

/**
 * @brief Copy image into arbitrary 2D accelerator path.
 *
 * This function leverages the 2D accelerator tesselation matrix and path opcode
 * definitions to enable the user to copy a source buffer into an arbitrary
 * gpu path. The 2D accelerator paths follow the same restrictions as @ref
 * accel2d_draw_path requires.
 * @param dev: 2D accelerator device object
 * @param dst_buf: 2D accelerator output buffer description
 * @param src_buf: 2D accelerator source buffer description
 * @param path: uint32_t array of ACCEL2D_OP opcode definitions.
 * @param path_len: length of path array, in bytes
 * @param fill_rule: fill rule for path.
 * @param path_transform: gpu affine transformation matrix for path
 * @param blit_transform: gpu affine transformation matrix for source buffer
 * @param pad_color: ARGB8888 color to be applied to region outside of
 *                   source buffer in path
 * @param blend: 2D accelerator porter-duff blend setting
 * @retval 0 command was queued successfully
 * @retval -ENOSYS if this 2D accelerator does not support this feature
 * @retval -ENOTSUP if the 2D accelerator does not support the
 *          requested combination of settings
 * @retval -EINVAL invalid buffer alignment settings for the 2D accelerator
 * @retval -EBUSY @ref accel2d_render must be called before this operation can
 *          proceed, as no more rendering can be queued on the 2D accelerator
 * hardware.
 * @retval -EIO Underlying 2D accelerator encountered an I/O error.
 */
int accel2d_blit_path(const struct device *dev, struct accel_buf *dst_buf,
		      struct accel_buf *src_buf, uint32_t *path,
		      uint32_t path_len, enum accel2d_fill_rule fill_rule,
		      struct accel2d_matrix path_transform,
		      struct accel2d_matrix blit_transform, uint32_t pad_color,
		      enum accel2d_blend blend);

/**
 * @brief Copy image into 2D accelerator output buffer.
 *
 * Note that this function is deliberately somewhat constrained in the arguments
 * it can accept- it is intended for use with pixel engines that support limited
 * rotation and scale operations.
 * @param dev: 2D accelerator device object
 * @param dst_buf: 2D accelerator output buffer description
 * @param src_buf: 2D accelerator source buffer description
 * @param src_coords: coordinates to write source buffer to.
 * @param rotation: 2D accelerator rotation setting.
 * @param scale_factor: scale setting to apply to 2D accelerator source buffer.
 * @param blend: 2D accelerator porter-duff blend setting
 * @retval 0 command was queued successfully
 * @retval -ENOSYS if this 2D accelerator does not support this feature
 * @retval -ENOTSUP if the 2D accelerator does not support the
 *          requested combination of settings
 * @retval -EINVAL invalid buffer alignment settings for the 2D accelerator
 * @retval -EBUSY @ref accel2d_render must be called before this operation can
 *          proceed, as no more rendering can be queued on the 2D accelerator
 * hardware.
 * @retval -EIO Underlying 2D accelerator encountered an I/O error.
 */
int accel2d_blit_img(const struct device *dev, struct accel_buf *dst_buf,
		     struct accel_buf *src_buf,
		     struct accel2d_coords *src_coords,
		     enum accel2d_rotate rotation,
		     enum accel2d_scale scale_factor, enum accel2d_blend blend);

/**
 * @brief Flush a 2D accelerator's render queue
 *
 * Flush a 2D accelerator's render queue. Upon return, all buffers
 * queued for this 2D accelerator in render commands should be updated
 * with render outputs.
 * @param dev: 2D accelerator device object
 * @retval 0 flush completed successfully
 * @retval -EIO Underlying 2D accelerator encountered an I/O error
 */
int accel2d_render(const struct device *dev);

/**
 * @brief Write directly to a display using the 2D accelerator.
 *
 * Write direct to an attached display using the 2D accelerator. This function
 * will write the provided framebuffer directly to the display, performing pixel
 * format conversions and transformations automatically.
 * @param dev: 2D accelerator device object
 * @param coords: output coordinates to write to on display
 * @param src_buf: 2D accelerator source buffer.
 * @param rotation: rotation setting to apply to buffer.
 * @param scale_factor: scale factor to apply before sending to display
 * @param output_format: output format for pixels to stream to display.
 * @retval 0 write completed successfully.
 * @retval -ENOSYS if this 2D accelerator does not support this feature
 * @retval -ENOTSUP if the 2D accelerator does not support the
 *          requested combination of settings
 * @retval -EINVAL invalid buffer alignment settings for the 2D accelerator
 * @retval -EBUSY @ref accel2d_render must be called before this operation can
 *          proceed, as no more rendering can be queued on the 2D accelerator
 * hardware.
 * @retval -EIO Underlying 2D accelerator encountered an I/O error.
 */
int accel2d_write_display(const struct device *dev,
			  struct accel2d_coords *coords,
			  struct accel_buf *src_buf,
			  enum accel2d_rotate rotation,
			  enum accel2d_scale scale_factor,
			  enum display_pixel_format output_format);

/*
 * @brief Read capabilities of 2D accelerator device
 *
 * Read capabilities of 2D accelerator device. This API can be used by graphics
 * stacks to determine which operations are supported by the 2D accelerator
 * hardware on their system, so they can fallback to software rendering where
 * required.
 * @param dev: 2D accelerator device object
 * @param caps: 2D accelerator capabilities struct, to be filled by the driver
 * @retval 0 capabilities were queried successfully
 * @retval -ENOSYS if the 2D accelerator does not support this API
 */
int accel2d_read_caps(const struct device *dev,
		      struct accel2d_capabilities *caps);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ACCEL2D_H_ */

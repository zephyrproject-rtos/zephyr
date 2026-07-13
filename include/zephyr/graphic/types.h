/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 STMicroelectronics
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup graphic_types
 * @brief Types used by both graphic drivers and subsystem
 */

#ifndef ZEPHYR_INCLUDE_GRAPHIC_TYPES_H
#define ZEPHYR_INCLUDE_GRAPHIC_TYPES_H

/**
 * @brief Graphic type definitions.
 * @defgroup graphic_types Graphic Types
 * @since 4.5
 * @version 0.0.1
 * @ingroup graphic_api
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/display.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief graphic_area struct
 *
 * Structure describing an area
 */
struct graphic_area {
	/** pixel format value, from enum display_pixel_format */
	enum display_pixel_format pixelformat;
	/** Width in pixel of the area */
	uint32_t width;
	/** Height in pixel of the area */
	uint32_t height;
	/** Stride, amount of pixels between the first pixel of 2 consecutive rows */
	uint32_t stride;
	/** Opacity of this area */
	uint8_t opacity;
	/** Address of data */
	uint8_t *addr;
};

/**
 * @brief graphic_filled_area struct
 *
 * Structure describing an area filled with a fixed color
 */
struct graphic_filled_area {
	/** Width in pixel of the area */
	uint32_t width;
	/** Height in pixel of the area */
	uint32_t height;
	/** Opacity of this area */
	uint8_t opacity;
	/** Red component value */
	uint8_t red;
	/** Green component value */
	uint8_t green;
	/** Blue component value */
	uint8_t blue;
};

/**
 * @brief graphic_output struct
 *
 * Structure describing the output area
 */
struct graphic_output_area {
	/** pixel format value, from enum display_pixel_format */
	enum display_pixel_format pixelformat;
	/** Stride, amount of pixels between the first pixel of 2 consecutive rows */
	uint32_t stride;
	/** Address of data */
	uint8_t *addr;
};

/**
 * @brief graphic_operation_status enum
 *
 * Status of an operation
 */
enum graphic_operation_status {
	/** Un-allocated operation */
	GRAPHIC_OP_UNALLOCATED = 0,
	/** Allocated operation */
	GRAPHIC_OP_ALLOCATED,
	/** Submitted operation */
	GRAPHIC_OP_SUBMITTED,
	/** Ongoing operation */
	GRAPHIC_OP_ONGOING,
	/** Canceled operation */
	GRAPHIC_OP_CANCELED,
	/** Successfully completed operation */
	GRAPHIC_OP_COMPLETED,
	/** Error completed operation */
	GRAPHIC_OP_ERROR,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_GRAPHIC_TYPES_H */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zvid plugin.
 */

#ifndef __MP_ZVID_H__
#define __MP_ZVID_H__

#include <stdint.h>

#include <zephyr/drivers/video.h>
#include <zephyr/sys/util.h>

#include <src/core/mp_structure.h>
#include <src/core/mp_value.h>

/**
 * @brief Pixel format mapping descriptor
 *
 * Maps between libmp pixel formats and Zephyr video driver pixel formats.
 */
typedef struct {
	/** libMP pixel format */
	MpPixelFormat mp_fmt;
	/** Zephyr video pixel format */
	uint32_t zvid_fmt;
} MpZvidPixfmtDesc;

/**
 * @brief Pixel format mapping table
 *
 * Static array mapping between @ref MpPixelFormat and Zephyr video formats.
 * Keep this array synchronized with Zephyr video formats.
 */
static const MpZvidPixfmtDesc mp_zvid_pixfmt_map[] = {
	{MP_PIXEL_FORMAT_YUYV, VIDEO_PIX_FMT_YUYV},
	{MP_PIXEL_FORMAT_XYUV32, VIDEO_PIX_FMT_XYUV32},
	{MP_PIXEL_FORMAT_RGB24, VIDEO_PIX_FMT_RGB24},
	{MP_PIXEL_FORMAT_RGB565, VIDEO_PIX_FMT_RGB565},
	{MP_PIXEL_FORMAT_XRGB32, VIDEO_PIX_FMT_XRGB32},
	{MP_PIXEL_FORMAT_ARGB32, VIDEO_PIX_FMT_ARGB32},
};

/**
 * @brief Convert Zephyr video format to libMP pixel format
 *
 * @param zvid_fmt Zephyr video pixel format
 *
 * @return Corresponding @ref MpPixelFormat, or MP_PIXEL_FORMAT_UNKNOWN if not found
 */
static const MpPixelFormat zvid2mp_pixfmt(uint32_t zvid_fmt)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_zvid_pixfmt_map); i++) {
		if (mp_zvid_pixfmt_map[i].zvid_fmt == zvid_fmt) {
			return mp_zvid_pixfmt_map[i].mp_fmt;
		}
	}

	return MP_PIXEL_FORMAT_UNKNOWN;
}

/**
 * @brief Convert libMP pixel format to Zephyr video format
 *
 * @param mp_fmt libMP pixel format from @ref MpPixelFormat
 *
 * @return Corresponding Zephyr video pixel format, or 0 if not found
 */
static const uint32_t mp2zvid_pixfmt(MpPixelFormat mp_fmt)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_zvid_pixfmt_map); i++) {
		if (mp_zvid_pixfmt_map[i].mp_fmt == mp_fmt) {
			return mp_zvid_pixfmt_map[i].zvid_fmt;
		}
	}

	return 0;
}

/**
 * @brief Extract Zephyr video format capabilities from libMP structure
 *
 * Parses an @ref MpStructure containing video format information and populates
 * a Zephyr video format capability structure. The function extracts:
 * - Pixel format (from "format" field)
 * - Width (from "width" field)
 * - Height (from "height" field)
 *
 * @param structure Pointer to @ref MpStructure containing format information
 * @param fcaps Pointer to video_format_cap structure to be populated
 *
 */
static void get_zvid_fmt_from_structure(MpStructure *structure, struct video_format_cap *fcaps)
{
	MpValue *value;

	/* Get pixel format field */
	value = mp_structure_get_value(structure, "format");
	if (value == NULL) {
		return;
	}
	if (value->type == MP_TYPE_UINT) {
		fcaps->pixelformat = mp2zvid_pixfmt(mp_value_get_uint(value));
	} else if (value->type == MP_TYPE_LIST) {
		/* Format may be of MP_TYPE_LIST due to the intersection with a list type but when
		 * converted to zvid format, it is supposed to be a single-value list, so take the
		 * 1st item in the list */
		fcaps->pixelformat = mp2zvid_pixfmt(mp_value_get_uint(mp_value_list_get(value, 0)));
	} else {
		return;
	}

	/* Get width fields */
	value = mp_structure_get_value(structure, "width");
	if (value == NULL) {
		return;
	}
	fcaps->width_min = value->type == MP_TYPE_INT_RANGE ? mp_value_get_int_range_min(value)
							    : mp_value_get_int(value);
	fcaps->width_max = value->type == MP_TYPE_INT_RANGE ? mp_value_get_int_range_max(value)
							    : fcaps->width_min;
	fcaps->width_step =
		value->type == MP_TYPE_INT_RANGE ? mp_value_get_int_range_step(value) : 0;

	/* Get height fields */
	value = mp_structure_get_value(structure, "height");
	if (value == NULL) {
		return;
	}
	fcaps->height_min = value->type == MP_TYPE_INT_RANGE ? mp_value_get_int_range_min(value)
							     : mp_value_get_int(value);
	fcaps->height_max = value->type == MP_TYPE_INT_RANGE ? mp_value_get_int_range_max(value)
							     : fcaps->height_min;
	fcaps->height_step =
		value->type == MP_TYPE_INT_RANGE ? mp_value_get_int_range_step(value) : 0;
}

#endif /* __MP_ZVID_H__ */

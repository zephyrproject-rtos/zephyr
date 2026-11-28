/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup video_interface
 * @brief HTPA colorizer controls.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_VIDEO_HTPA_COLORIZER_H_
#define ZEPHYR_INCLUDE_DRIVERS_VIDEO_HTPA_COLORIZER_H_

#include <zephyr/video/controls.h>

/**
 * @defgroup video_htpa_colorizer HTPA colorizer
 * @ingroup video_interface
 * @{
 */

/** Palette used for RGB565 output; value is an enum video_htpa_palette. */
#define VIDEO_CID_HTPA_PALETTE (VIDEO_CID_PRIVATE_BASE + 1U)

/** False-color palette selection. */
enum video_htpa_palette {
	VIDEO_HTPA_PALETTE_IRONBOW = 0, /**< Ironbow palette. */
	VIDEO_HTPA_PALETTE_RAINBOW = 1, /**< Rainbow palette. */
	VIDEO_HTPA_PALETTE_INFERNO = 2, /**< Inferno palette. */
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_VIDEO_HTPA_COLORIZER_H_ */

/*
 * Copyright (c) 2026 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_VIDEO_HM_HTPA_VIDEO_HTPA_CALIB_H_
#define ZEPHYR_DRIVERS_VIDEO_VIDEO_HM_HTPA_VIDEO_HTPA_CALIB_H_

#include <stdint.h>

struct video_buffer;

struct htpa_calib {
	uint8_t mbit_calib;
	uint8_t bias_calib;
	uint8_t clk_calib;
	uint8_t bpa_calib;
	uint8_t pu_calib;
	uint8_t mbit_user;
	uint8_t bias_user;
	uint8_t clk_user;
	uint8_t bpa_user;
	uint8_t pu_user;
	uint32_t id;
};

/**
 * @brief Apply sensor calibration to a video frame.
 *
 * @param calib Sensor calibration data.
 * @param frame Video frame to calibrate.
 */
void htpa_apply_calib(const struct htpa_calib *calib, struct video_buffer *frame);

#endif /* ZEPHYR_DRIVERS_VIDEO_VIDEO_HM_HTPA_VIDEO_HTPA_CALIB_H_ */

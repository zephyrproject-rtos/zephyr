/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * Based on ST7789V sample:
 * Copyright (c) 2019 Marc Reilly
 *
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAMPLE_DISPLAY_H__
#define __SAMPLE_DISPLAY_H__

#define SAMPLE_DISPLAY_NUM_FULL_FRAMEBUFFERS \
	(IS_ENABLED(CONFIG_SAMPLE_DISPLAY_DOUBLE_BUFFER) ? 2 : 1)

int sample_display_draw(void);

#endif

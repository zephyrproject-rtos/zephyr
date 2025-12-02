/*
 * Copyright 2019, 2024-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _VGLITE_WINDOW_H_
#define _VGLITE_WINDOW_H_

#include "vg_lite.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define FRAME_BUFFER_ALIGN          32
#define APP_BUFFER_COUNT            2
#define VG_LITE_COMMAND_BUFFER_SIZE (256 << 10) /* 256 KB */

#if defined(CONFIG_SOC_MIMXRT1176_CM7)
#define USE_PSRAM_FRAMEBUFFER 0
#elif defined(CONFIG_SOC_MIMXRT595S_CM33)
#define USE_PSRAM_FRAMEBUFFER 1
#else
#error "Unsupported SoC!"
#endif

/* Default tessellation window width and height, in pixels */
#define DEFAULT_VG_LITE_TW_WIDTH  128 /* pixels */
#define DEFAULT_VG_LITE_TW_HEIGHT 128 /* pixels */

typedef struct {
	const struct device *dev;
	uint16_t width;
	uint16_t height;
} vg_lite_display_t;

typedef struct vg_lite_window {
	vg_lite_display_t *display;
	vg_lite_buffer_t buffers[APP_BUFFER_COUNT];
	int width;
	int height;
	int bufferCount;
	int current;
} vg_lite_window_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

vg_lite_error_t VGLITE_CreateWindow(vg_lite_display_t *display, vg_lite_window_t *window);

vg_lite_buffer_t *VGLITE_GetRenderTarget(vg_lite_window_t *window);

void VGLITE_SwapBuffers(vg_lite_window_t *window);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* _VGLITE_WINDOW_H_ */

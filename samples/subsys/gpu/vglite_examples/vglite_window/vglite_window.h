/*
 * Copyright 2019, 2024-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _VGLITE_WINDOW_H_
#define _VGLITE_WINDOW_H_
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/display/panel.h>
#include <zephyr/sys/util.h>
#include "vg_lite.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define FRAME_BUFFER_ALIGN          32
#define APP_BUFFER_COUNT            2
#define VG_LITE_COMMAND_BUFFER_SIZE (256 << 10) /* 256 KB */

/* Get display node */
#define DISPLAY_NODE DT_CHOSEN(zephyr_display)

#if !DT_NODE_HAS_STATUS(DISPLAY_NODE, okay)
    #error "Display node not found. Ensure 'zephyr,display' is set in devicetree chosen node."
#endif

/* Get display dimensions */
#define DISPLAY_WIDTH  DT_PROP(DISPLAY_NODE, width)
#define DISPLAY_HEIGHT DT_PROP(DISPLAY_NODE, height)

/* Get pixel format */
#define DISPLAY_PIXEL_FORMAT DT_PROP(DISPLAY_NODE, pixel_format)

#if DISPLAY_PIXEL_FORMAT == PANEL_PIXEL_FORMAT_RGB_565
    #define DISPLAY_BYTES_PER_PIXEL 2
    #define PIXEL_FORMAT_NAME "RGB565"
#elif DISPLAY_PIXEL_FORMAT == PANEL_PIXEL_FORMAT_RGB_888
    #define DISPLAY_BYTES_PER_PIXEL 3
    #define PIXEL_FORMAT_NAME "RGB888"
#elif DISPLAY_PIXEL_FORMAT == PANEL_PIXEL_FORMAT_ARGB_8888
    #define DISPLAY_BYTES_PER_PIXEL 4
    #define PIXEL_FORMAT_NAME "ARGB8888"
#else
    #warning "Unknown pixel format, defaulting to RGB565"
    #define DISPLAY_BYTES_PER_PIXEL 2
    #define PIXEL_FORMAT_NAME "RGB565 (default)"
#endif

/* Calculate framebuffer size */
#define FRAME_BUFFER_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_BYTES_PER_PIXEL)

/* Compile-time checks */
BUILD_ASSERT(DISPLAY_WIDTH > 0, "Invalid display width");
BUILD_ASSERT(DISPLAY_HEIGHT > 0, "Invalid display height");
BUILD_ASSERT(FRAME_BUFFER_SIZE > 0, "Invalid framebuffer size");

#if defined(CONFIG_SOC_MIMXRT1176_CM7)
#define USE_PSRAM_FRAMEBUFFER 0
#elif defined(CONFIG_SOC_MIMXRT595S_CM33) || defined(CONFIG_SOC_MIMXRT798S_CM33_CPU0)
#define USE_PSRAM_FRAMEBUFFER 1
#else
#error "Unsupported SoC!"
#endif

/* Default tessellation window width and height, in pixels */
#define DEFAULT_VG_LITE_TW_WIDTH  128 /* pixels */
#define DEFAULT_VG_LITE_TW_HEIGHT DISPLAY_HEIGHT /* pixels */


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

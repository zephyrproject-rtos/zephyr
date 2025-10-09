/*
 * Copyright 2019, 2024-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "vglite_window.h"
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

__aligned(FRAME_BUFFER_ALIGN) static uint8_t s_frameBuffer[APP_BUFFER_COUNT][720 * 1280 * 2];

vg_lite_error_t VGLITE_CreateWindow(vg_lite_display_t *display, vg_lite_window_t *window)
{
	vg_lite_error_t ret = VG_LITE_SUCCESS;

	window->bufferCount = APP_BUFFER_COUNT;
	window->display = display;
	window->width = display->width;
	window->height = display->height;
	window->current = -1;

	for (int i = 0; i < APP_BUFFER_COUNT; i++) {
		vg_lite_buffer_t *vg_buffer = &window->buffers[i];

		vg_buffer->memory = (void *)s_frameBuffer[i];
		vg_buffer->address = (uint32_t)s_frameBuffer[i];
		vg_buffer->width = display->width;
		vg_buffer->height = display->height;
		vg_buffer->stride = display->width * 2; /* RGB565 */
		vg_buffer->format = VG_LITE_BGR565;
		vg_buffer->tiled = VG_LITE_LINEAR;

		memset(vg_buffer->memory, 0, vg_buffer->height * vg_buffer->stride);
	}

	return ret;
}

vg_lite_buffer_t *VGLITE_GetRenderTarget(vg_lite_window_t *window)
{
	if (!window) {
		return NULL;
	}

	window->current = (window->current + 1) % window->bufferCount;
	return &(window->buffers[window->current]);
}

/*
 * Swap buffers by calling display_write() with a CPU-accessible framebuffer.
 * We assume rt->memory is CPU RAM (allocated above). If your VG-Lite cannot
 * render into that region, this will not work.
 */
void VGLITE_SwapBuffers(vg_lite_window_t *window)
{
	vg_lite_buffer_t *rt;

	if (window->current >= 0 && window->current < window->bufferCount) {
		rt = &(window->buffers[window->current]);
	} else {
		return;
	}

	vg_lite_finish(); /* ensure GPU work is done */
	size_t fb_size = rt->stride * rt->height;

	struct display_buffer_descriptor desc = {
		.width = rt->width,
		.height = rt->height,
		.pitch = rt->width,
		.buf_size = fb_size,
	};

	desc.frame_incomplete = true;

	/* Use display_write(); rt->memory must be CPU-accessible for this to be safe */
	int err = display_write(window->display->dev, 0, 0, &desc, rt->memory);

	if (err) {
		printk("display_write() failed: %d\n", err);
		return;
	}
}

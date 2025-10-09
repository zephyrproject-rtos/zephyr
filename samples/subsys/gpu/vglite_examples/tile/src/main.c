/*
 * Copyright 2019, 2021, 2024-2025 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include "vglite_window.h"
/*-----------------------------------------------------------*/
#include "vg_lite.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define VGLITE_STACK_SIZE 			CONFIG_VGLITE_STACK_SIZE
#define VGLITE_PRIORITY   			K_HIGHEST_THREAD_PRIO

#if USE_PSRAM_FRAMEBUFFER == 0
__aligned(FRAME_BUFFER_ALIGN) static uint16_t tiled_fb_mem[APP_BUFFER_COUNT][DISPLAY_WIDTH * DISPLAY_HEIGHT];
#else
__aligned(128) __section(".lvgl_buf") uint16_t tiled_fb_mem[DISPLAY_WIDTH * DISPLAY_HEIGHT];
#endif

K_THREAD_STACK_DEFINE(vglite_stack, VGLITE_STACK_SIZE);
static struct k_thread vglite_thread;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void vglite_task(void *arg1, void *arg2, void *arg3);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static vg_lite_display_t display;
static vg_lite_window_t window;

/*
	    *-----*
	   /       \
	  /         \
	 *           *
	 |          /
	 |         X
	 |          \
	 *           *
	  \         /
	   \       /
	    *-----*
 */
static int8_t path_data[] = {
	2, -5,  -10, /* moveto   -5, -10 */
	4, 5,   -10, /* lineto    5, -10 */
	4, 10,  -5,  /* lineto   10,  -5 */
	4, 0,   0,   /* lineto    0,   0 */
	4, 10,  5,   /* lineto   10,   5 */
	4, 5,   10,  /* lineto    5,  10 */
	4, -5,  10,  /* lineto   -5, 10 */
	4, -10, 5,   /* lineto  -10,  5 */
	4, -10, -5,  /* lineto  -10, -5 */
	0,           /* end */
};

static vg_lite_path_t path = {
	{
		-10, -10, /* left, top */
		10, 10    /* right, bottom */
	},
	VG_LITE_HIGH,      /* quality */
	VG_LITE_S8,        /* -128 to 127 coordinate range */
	{0},               /* uploaded */
	sizeof(path_data), /* path length */
	path_data,         /* path data */
	1                  /* path type or reserved flag */
};

static vg_lite_matrix_t matrix;
static vg_lite_buffer_t tiled_buffer;

/*******************************************************************************
 * Code
 ******************************************************************************/

int main(void)
{
	k_tid_t tid =
		k_thread_create(&vglite_thread, vglite_stack, K_THREAD_STACK_SIZEOF(vglite_stack),
				vglite_task, NULL, NULL, NULL, VGLITE_PRIORITY, 0, K_NO_WAIT);

	printk("Thread created, tid=%p\n", tid);

	while (1) {
		k_sleep(K_MSEC(1000));
	}
}

static void cleanup(void)
{
	vg_lite_clear_path(&path);
	vg_lite_close();
}

static vg_lite_error_t init_vg_lite(vg_lite_display_t *display)
{
	vg_lite_error_t error = VG_LITE_SUCCESS;

	/* Initialize the window. */
	error = VGLITE_CreateWindow(display, &window);
	if (error) {
		printk("VGLITE_CreateWindow failed: VGLITE_CreateWindow() returned error %d\n",
		       error);
		return error;
	}
	/* Set GPU command buffer size for this drawing task. */
	error = vg_lite_set_command_buffer_size(VG_LITE_COMMAND_BUFFER_SIZE);
	if (error) {
		printk("vg_lite_set_command_buffer_size() returned error %d\n", error);
		cleanup();
		return error;
	}
	/* Initialize the draw. */
	error = vg_lite_init(DEFAULT_VG_LITE_TW_WIDTH, DEFAULT_VG_LITE_TW_HEIGHT);
	if (error) {
		printk("vg_lite engine init failed: vg_lite_init() returned error %d\n", error);
		cleanup();
		return error;
	}

	/* Initialize tiled buffer */
	memset(&tiled_buffer, 0, sizeof(tiled_buffer));
	tiled_buffer.width = display->width;
	tiled_buffer.height = display->height;
	tiled_buffer.stride = display->width * 2;
	tiled_buffer.tiled = VG_LITE_TILED;
	tiled_buffer.format = VG_LITE_RGB565;
	tiled_buffer.memory = (void *)tiled_fb_mem;
	tiled_buffer.address = tiled_fb_mem;

	/* Align stride to 64 bytes (required for the tiled raster images) */
	if (tiled_buffer.stride & 0x3f) {
		tiled_buffer.stride = (tiled_buffer.stride & (~(uint32_t)0x3f)) + 64;
	}

	vg_lite_clear(&tiled_buffer, NULL, 0x0);

	return error;
}

static void redraw()
{
	vg_lite_error_t error = VG_LITE_SUCCESS;

	vg_lite_buffer_t *rt = VGLITE_GetRenderTarget(&window);
	if (rt == NULL) {
		printk("vg_lite_get_renderTarget error\r\n");
		while (1)
			;
	}
	vg_lite_identity(&matrix);
	vg_lite_translate(window.width / 2.0f, window.height / 2.0f, &matrix);
	vg_lite_scale(10, 10, &matrix);

	/* Draw the path using the matrix. */
	error = vg_lite_draw(&tiled_buffer, &path, VG_LITE_FILL_EVEN_ODD, &matrix,
			     VG_LITE_BLEND_NONE, 0xFF0000FFU);
	if (error) {
		printk("vg_lite_draw() returned error %d\n", error);
		cleanup();
		return;
	}

	vg_lite_clear(rt, NULL, 0x0);
	vg_lite_identity(&matrix);
	error = vg_lite_blit(rt, &tiled_buffer, &matrix, VG_LITE_BLEND_NONE, 0,
			     VG_LITE_FILTER_BI_LINEAR);
	if (error) {
		printk("vg_lite_blit() returned error %d\n", error);
		cleanup();
		return;
	}

	VGLITE_SwapBuffers(&window);

	return;
}

uint32_t getTime()
{
	return k_uptime_get_32(); /* milliseconds */
}

static void vglite_task(void *arg1, void *arg2, void *arg3)
{
	vg_lite_error_t error;
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		printk("display device not ready\n");
		return;
	}

	display_blanking_off(display_dev);
	struct display_capabilities capabilities;

	display_get_capabilities(display_dev, &capabilities);

	vg_lite_display_t my_display = {.dev = display_dev,
					.width = capabilities.x_resolution,
					.height = capabilities.y_resolution};

	error = init_vg_lite(&my_display);
	if (error) {
		printk("init_vg_lite failed: init_vg_lite() returned error %d\n", error);
		while (1)
			;
	}

	uint32_t startTime, time, n = 0;
	startTime = getTime();
	while (1) {
		redraw();
		n++;
		if (n >= 60) {
			time = getTime() - startTime;
			printk("%d frames in %d seconds: %d fps\r\n", n, time / 1000,
			       n * 1000 / time);
			n = 0;
			startTime = getTime();
		}
	}
}

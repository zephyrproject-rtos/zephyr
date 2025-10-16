/*
 * Copyright 2019, 2021, 2023-2025 NXP
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

#include "draw_cube.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define VGLITE_STACK_SIZE 			CONFIG_VGLITE_STACK_SIZE
#define VGLITE_PRIORITY   			K_HIGHEST_THREAD_PRIO
#define DEFAULT_SIZE      256.0f;

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
	vg_lite_close();
}

static vg_lite_error_t init_vg_lite(vg_lite_display_t *display)
{
	vg_lite_error_t error = VG_LITE_SUCCESS;

	/* Initialize the window */
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
	/* Initialize the draw */
	error = vg_lite_init(DEFAULT_VG_LITE_TW_WIDTH, DEFAULT_VG_LITE_TW_HEIGHT);
	if (error) {
		printk("vg_lite engine init failed: vg_lite_init() returned error %d\n", error);
		cleanup();
		return error;
	}

	if (!load_texture_images()) {
		printk("load texture images failed: load_texture_images() returned false\n");
		cleanup();
		return VG_LITE_INVALID_ARGUMENT;
	}

	return error;
}

static void redraw(void)
{
	vg_lite_buffer_t *rt = VGLITE_GetRenderTarget(&window);

	if (rt == NULL) {
		printk("vg_lite_get_renderTarget error\r\n");
		while (1) {
		}
	}

	/* Clear the buffer with black color */
	vg_lite_clear(rt, NULL, 0x00000000);

	/* Draw cube */
	draw_cube(rt);

	vg_lite_finish();

	VGLITE_SwapBuffers(&window);
}

uint32_t getTime(void)
{
	return k_uptime_get_32(); /* milliseconds */
}

static void vglite_task(void *arg1, void *arg2, void *arg3)
{
	status_t status;
	vg_lite_error_t error;
	uint32_t startTime, time, n = 0, fps_x_1000;
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
		while (1) {
		}
	}

	startTime = getTime();
	while (1) {
		redraw();
		n++;
		if (n >= 60) {
			time = getTime() - startTime;
			fps_x_1000 = (n * 1000 * 1000) / time;
			printk("%d frames in %d mSec: %d.%d FPS\r\n", n, time, fps_x_1000 / 1000,
			       fps_x_1000 % 1000);
			n = 0;
			startTime = getTime();
		}
	}
}

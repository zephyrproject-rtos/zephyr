/*
 * Copyright 2019, 2024-2025 NXP
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
#include "tiger_paths.h"
/*-----------------------------------------------------------*/
#include "vg_lite.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define VGLITE_STACK_SIZE 			CONFIG_VGLITE_STACK_SIZE
#define VGLITE_PRIORITY   			K_HIGHEST_THREAD_PRIO
#define VGLITE_COMMAND_BUFFER_SZ 	(128 * 1024)

#if (720 * 1280 == (DEMO_PANEL_WIDTH) * (DEMO_PANEL_HEIGHT))
#define TW 720
/* On RT595S */
#if defined(CONFIG_SOC_MIMXRT595S_CM33)
/* Tessellation window = 720 x 128 */
#define TH 128
/* On RT798S */
#elif defined(CONFIG_SOC_MIMXRT798S_CM33_CPU0)
/* Tessellation window = 720 x 640 */
#define TH 640
/* On RT1170 */
#elif defined(CONFIG_SOC_MIMXRT1176_CM7) || defined(CONFIG_SOC_MIMXRT1166_CM7)
/* Tessellation window = 720 x 1280 */
#define TH 1280
#else
#error "Unsupported CPU !"
#endif
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

static int zoomOut;
static int scaleCount;
static vg_lite_matrix_t matrix;

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
	uint8_t i;

	for (i = 0; i < pathCount; i++) {
		vg_lite_clear_path(&path[i]);
	}

	vg_lite_close();
}

static vg_lite_error_t init_vg_lite(vg_lite_display_t *display)
{
	vg_lite_error_t error = VG_LITE_SUCCESS;
	int fb_width, fb_height;

	/* Initialize the window. */
	error = VGLITE_CreateWindow(display, &window);
	if (error) {
		printk("VGLITE_CreateWindow failed: VGLITE_CreateWindow() returned error %d\n",
		       error);
		return error;
	}

	/* Initialize the draw. */
	error = vg_lite_init(DEFAULT_VG_LITE_TW_WIDTH, DEFAULT_VG_LITE_TW_HEIGHT);
	if (error) {
		printk("vg_lite engine init failed: vg_lite_init() returned error %d\n", error);
		cleanup();
		return error;
	}

	/* Set GPU command buffer size for this drawing task. */
	error = vg_lite_set_command_buffer_size(VGLITE_COMMAND_BUFFER_SZ);
	if (error) {
		printk("vg_lite_set_command_buffer_size() returned error %d\n", error);
		cleanup();
		return error;
	}

	/* Setup a scale at center of buffer. */
	fb_width = window.width;
	fb_height = window.height;
	vg_lite_identity(&matrix);
	vg_lite_translate(fb_width / 2 - 20 * fb_width / 640.0f,
			  fb_height / 2 - 100 * fb_height / 480.0f, &matrix);
	vg_lite_scale(2, 2, &matrix);
	vg_lite_scale(fb_width / 640.0f, fb_height / 480.0f, &matrix);

	return error;
}

void animateTiger(void)
{
	if (zoomOut) {
		vg_lite_scale(1.25, 1.25, &matrix);
		if (0 == --scaleCount) {
			zoomOut = 0;
		}
	} else {
		vg_lite_scale(0.8, 0.8, &matrix);
		if (5 == ++scaleCount) {
			zoomOut = 1;
		}
	}

	vg_lite_rotate(5, &matrix);
}

static void redraw(void)
{
	vg_lite_error_t error = VG_LITE_SUCCESS;
	uint8_t count;
	vg_lite_buffer_t *rt = VGLITE_GetRenderTarget(&window);

	if (rt == NULL) {
		printk("vg_lite_get_renderTarget error\r\n");
		while (1) {
		}
	}

	/* Draw the path using the matrix. */
	vg_lite_clear(rt, NULL, 0xFFFFFFFFU);
	for (count = 0; count < pathCount; count++) {
		error = vg_lite_draw(rt, &path[count], VG_LITE_FILL_EVEN_ODD, &matrix,
				     VG_LITE_BLEND_NONE, color_data[count]);
		if (error) {
			printk("vg_lite_draw() returned error %d\r\n", error);
			cleanup();
			return;
		}
	}

	VGLITE_SwapBuffers(&window);

	animateTiger();
}

uint32_t getTime(void)
{
	return k_uptime_get_32(); /* milliseconds */
}

static void vglite_task(void *arg1, void *arg2, void *arg3)
{
	status_t status;
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
		while (1) {
		}
	}

	uint32_t startTime, time, n = 0, fps_x_1000;

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

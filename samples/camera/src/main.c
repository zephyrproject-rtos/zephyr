/*
 * Copyright (c) 2019 NXP Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/camera.h>
#include <drivers/display.h>
#include <stdio.h>
#include <misc/__assert.h>

/* _______          _________
 *|       |<-------|head|    |
 *|       |	   |____|    |___________      ______
 *|	  |	   |tail|<---|__camer_IP_|<---|_CMOS_|
 *|	  |------->|____|____|
 *|_______|	   |____|____|
 * Display	      Camera
 */

#define CAMERA_DEMO_FB_MAX_SIZE 0x40000
#define CAMERA_DEMO_FB_MAX_NUM 4
char gfb[CAMERA_DEMO_FB_MAX_SIZE * CAMERA_DEMO_FB_MAX_NUM] __aligned(64);
struct k_sem gsem;

static void capture_cb(void *fb, int w, int h, int bpp)
{
	k_sem_give(&gsem);
}

void main(void)
{
	int ret, i;
	struct device *camera_dev = camera_get_primary();
	struct device *display_dev =
		device_get_binding(CONFIG_CAMERA_DISPLAY_DEV_NAME);
	struct display_capabilities diaplay_cap;
	struct display_buffer_descriptor diaplay_dsc;
	struct camera_capability camera_cap;
	struct camera_fb_cfg camera_cfg;
	void *fb[CAMERA_DEMO_FB_MAX_NUM];
	u8_t bpp;
	s64_t start_tick;
	double frames = 0;
	double ms;

	k_sem_init(&gsem, 0, 1);
	if (camera_dev == NULL) {
		printf("Failed to get camera device.\n");
		return;
	}

	ret = camera_power(camera_dev, true);
	if (!ret) {
		printf("%s camera power on successfully!\r\n",
			camera_dev->config->name);
	} else {
		printf("%s camera power on failed!\r\n",
			camera_dev->config->name);

		return;
	}

	/*camera_reset is option, just for reset API test.*/
	ret = camera_reset(camera_dev);
	if (!ret) {
		printf("%s camera reset successfully!\r\n",
			camera_dev->config->name);
	} else {
		printf("%s camera reset failed!\r\n",
			camera_dev->config->name);

		return;
	}

	ret = camera_get_cap(camera_dev, &camera_cap);
	if (!ret) {
		printf("%s camera get capability successfully!\r\n",
			camera_dev->config->name);
	} else {
		printf("%s camera get capability failed!\r\n",
			camera_dev->config->name);

		return;
	}

	if (display_dev == NULL) {
		printf("Failed to get display device.\n");
	} else {
		display_get_capabilities(display_dev, &diaplay_cap);
	}

	for (i = 0; i < CAMERA_DEMO_FB_MAX_NUM; i++) {
		fb[i] = &gfb[i * CAMERA_DEMO_FB_MAX_SIZE];
	}

	if (display_dev) {
		if (diaplay_cap.current_pixel_format ==
			PIXEL_FORMAT_RGB_888) {
			bpp = 3;
		} else if (diaplay_cap.current_pixel_format ==
			PIXEL_FORMAT_ARGB_8888) {
			bpp = 4;
		} else if (diaplay_cap.current_pixel_format ==
			PIXEL_FORMAT_RGB_565) {
			bpp = 2;
		} else {
			printf("Unsupported pix format %d for camera\n",
				diaplay_cap.current_pixel_format);
			return;
		}

		if (!(camera_cap.pixformat_support &
			diaplay_cap.current_pixel_format)) {
			printf("The display pixel format 0x%08x is not"
				" supported by camera format supported: 0x%08x\r\n",
				diaplay_cap.current_pixel_format,
				camera_cap.pixformat_support);

			return;
		}

		if (diaplay_cap.x_resolution > camera_cap.width_max ||
			diaplay_cap.y_resolution > camera_cap.height_max) {
			printf("The display resolution exceeds"
				" the camera max resolution %d X %d\r\n",
				camera_cap.width_max, camera_cap.height_max);

			return;
		}

		camera_cfg.cfg_mode = CAMERA_USER_CFG;
		camera_cfg.fb_attr.width = diaplay_cap.x_resolution;
		camera_cfg.fb_attr.height = diaplay_cap.y_resolution;
		camera_cfg.fb_attr.bpp = bpp;
		camera_cfg.fb_attr.pixformat = diaplay_cap.current_pixel_format;

		diaplay_dsc.buf_size = camera_cfg.fb_attr.width *
			camera_cfg.fb_attr.height * bpp;
		diaplay_dsc.width = camera_cfg.fb_attr.width;
		diaplay_dsc.height = camera_cfg.fb_attr.height;
		diaplay_dsc.pitch = camera_cfg.fb_attr.width;
	} else {
		camera_cfg.cfg_mode = CAMERA_DEFAULT_CFG;
	}

	ret = camera_configure(camera_dev, &camera_cfg);
	if (!ret) {
		printf("%s camera configure successfully!\r\n",
			camera_dev->config->name);
	} else {
		printf("%s camera configure failed!\r\n",
			camera_dev->config->name);
	}

loop_again:
	printf("Enter capture mode\r\n");
	start_tick = z_tick_get();
	while (1) {
		void *camera_fb;

		ret = camera_start(camera_dev,
				CAMERA_CAPTURE_MODE, &fb[0], 1, capture_cb);
		if (!ret) {
			k_sem_take(&gsem, K_FOREVER);
			ret = camera_acquire_fb(camera_dev,
				&camera_fb, K_FOREVER);
			if (!ret) {
				if (display_dev) {
					display_write(display_dev, 0, 0,
						&diaplay_dsc, camera_fb);
				}
				frames++;
				printf("%d", (int)frames);
				printf("\b");
				if (frames >= 10) {
					printf("\b");
				}
				if (frames >= 100) {
					printf("\b");
				}
				if (unlikely(frames == 300)) {
					ms = __ticks_to_ms(
						z_tick_get() - start_tick);
					printf("\r\nCapture mode FPS: %f\r\n",
						(frames / (ms / 1000)));
					start_tick = z_tick_get();
					frames = 0;
					break;
				}
			} else {
				printf("Camera get frame buffer failed!\r\n");
			}
		} else {
			printf("Camera capture failed!\r\n");
		}
	}

	printf("Enter preview mode\r\n");
	ret = camera_start(camera_dev, CAMERA_PREVIEW_MODE, &fb[0],
				CAMERA_DEMO_FB_MAX_NUM, 0);
	if (ret) {
		printf("Preview start failed\r\n");
		goto loop_again;
	}

	start_tick = z_tick_get();
	while (1) {
		void *camera_fb;

		ret = camera_acquire_fb(camera_dev, &camera_fb, K_FOREVER);
		if (!ret) {
			if (display_dev) {
				display_write(display_dev, 0, 0,
					&diaplay_dsc, camera_fb);
			}

			frames++;
			printf("%d", (int)frames);
			printf("\b");
			if (frames >= 10) {
				printf("\b");
			}
			if (frames >= 100) {
				printf("\b");
			}
			if (unlikely(frames == 300)) {
				ms = __ticks_to_ms(
					z_tick_get() - start_tick);
				printf("\r\nPreview mode FPS: %f\r\n",
					(frames / (ms / 1000)));
				start_tick = z_tick_get();
				frames = 0;
				break;
			}

			camera_release_fb(camera_dev, camera_fb);
		} else {
			printf("Preview get frame buffer failed!\r\n");
		}
	}

	goto loop_again;
}

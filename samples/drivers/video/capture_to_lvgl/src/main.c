/*
 * Copyright (c) 2024 Charles Dias <charlesdias.cd@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>
#include <lvgl.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define VIDEO_DEV_SW "VIDEO_SW_GENERATOR"

int main(void)
{
	struct video_buffer *buffers[2], *vbuf;
	const struct device *display_dev;
	struct video_format fmt;
	struct video_caps caps;
	const struct device *video_dev;
	size_t bsize;
	int i = 0;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

#if DT_HAS_CHOSEN(zephyr_camera)
	video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));
	if (!device_is_ready(video_dev)) {
		LOG_ERR("%s device is not ready", video_dev->name);
		return 0;
	}
#else
	video_dev = device_get_binding(VIDEO_DEV_SW);
	if (video_dev == NULL) {
		LOG_ERR("%s device not found", VIDEO_DEV_SW);
		return 0;
	}
#endif

	LOG_INF("- Device name: %s", video_dev->name);

	/* Get capabilities */
	if (video_get_caps(video_dev, VIDEO_EP_OUT, &caps)) {
		LOG_ERR("Unable to retrieve video capabilities");
		return 0;
	}

	LOG_INF("- Capabilities:");
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		/* four %c to string */
		LOG_INF("  %c%c%c%c width [%u; %u; %u] height [%u; %u; %u]",
			(char)fcap->pixelformat, (char)(fcap->pixelformat >> 8),
			(char)(fcap->pixelformat >> 16), (char)(fcap->pixelformat >> 24),
			fcap->width_min, fcap->width_max, fcap->width_step, fcap->height_min,
			fcap->height_max, fcap->height_step);
		i++;
	}

	/* Get default/native format */
	if (video_get_format(video_dev, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return 0;
	}

	/* Set format */
	fmt.width = CONFIG_VIDEO_WIDTH;
	fmt.height = CONFIG_VIDEO_HEIGHT;
	fmt.pitch = fmt.width * 2;
	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;

	if (video_set_format(video_dev, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to set up video format");
		return 0;
	}

	LOG_INF("- Format: %c%c%c%c %ux%u %u", (char)fmt.pixelformat, (char)(fmt.pixelformat >> 8),
		(char)(fmt.pixelformat >> 16), (char)(fmt.pixelformat >> 24), fmt.width, fmt.height,
		fmt.pitch);

	/* Size to allocate for each buffer */
	bsize = fmt.pitch * fmt.height;

	/* Alloc video buffers and enqueue for capture */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		buffers[i] = video_buffer_alloc(bsize);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return 0;
		}

		video_enqueue(video_dev, VIDEO_EP_OUT, buffers[i]);
	}

#ifdef CONFIG_VIDEO_HFLIP
	/* Video flip image horizontally */
	if (video_set_ctrl(video_dev, VIDEO_CID_HFLIP, (void *)1)) {
		LOG_ERR("Unable to set video control (HFLIP)");
		return 0;
	}
#endif

#ifdef CONFIG_VIDEO_VFLIP
	/* Video flip image vertically */
	if (video_set_ctrl(video_dev, VIDEO_CID_VFLIP, (void *)1)) {
		LOG_ERR("Unable to set video control (VFLIP)");
		return 0;
	}
#endif

	/* Start video capture */
	if (video_stream_start(video_dev)) {
		LOG_ERR("Unable to start capture (interface)");
		return 0;
	}

	display_blanking_off(display_dev);

	const lv_img_dsc_t video_img = {
		.header.always_zero = 0,
		.header.w = CONFIG_VIDEO_WIDTH,
		.header.h = CONFIG_VIDEO_HEIGHT,
		.data_size = CONFIG_VIDEO_WIDTH * CONFIG_VIDEO_HEIGHT * sizeof(lv_color_t),
		.header.cf = LV_IMG_CF_TRUE_COLOR,
		.data = (const uint8_t *)buffers[0]->buffer,
	};

	lv_obj_t *screen = lv_img_create(lv_scr_act());

	LOG_INF("- Capture started");

	/* Grab video frames */
	while (1) {
		int err;

		err = video_dequeue(video_dev, VIDEO_EP_OUT, &vbuf, K_FOREVER);
		if (err) {
			LOG_ERR("Unable to dequeue video buf");
			return 0;
		}

		lv_img_set_src(screen, &video_img);
		lv_obj_align(screen, LV_ALIGN_BOTTOM_LEFT, 0, 0);

		lv_task_handler();

		err = video_enqueue(video_dev, VIDEO_EP_OUT, vbuf);
		if (err) {
			LOG_ERR("Unable to requeue video buf");
			return 0;
		}
	}
}

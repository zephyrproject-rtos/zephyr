/*
 * Copyright (c) 2024 Charles Dias <charlesdias.cd@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#if !DT_HAS_CHOSEN(zephyr_camera)
#error No camera chosen in devicetree. Missing "--shield" or "--snippet video-sw-generator" flag?
#endif

#if !DT_HAS_CHOSEN(zephyr_display)
#error No display chosen in devicetree. Missing "--shield" flag?
#endif

int main(void)
{
	struct video_buffer *buffers[2];
	struct video_buffer *vbuf = &(struct video_buffer){};
	const struct device *display_dev;
	const struct device *video_dev;
	struct video_format fmt;
	struct video_caps caps;
	enum video_buf_type type = VIDEO_BUF_TYPE_OUTPUT;
	struct video_selection sel = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};
	int i = 0;
	int err;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));
	if (!device_is_ready(video_dev)) {
		LOG_ERR("%s device is not ready", video_dev->name);
		return 0;
	}

	LOG_INF("- Device name: %s", video_dev->name);

	/* Get capabilities */
	caps.type = type;
	if (video_get_caps(video_dev, &caps)) {
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
	fmt.type = type;
	if (video_get_format(video_dev, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return 0;
	}

	/* Set the crop setting if necessary */
#if CONFIG_VIDEO_SOURCE_CROP_WIDTH && CONFIG_VIDEO_SOURCE_CROP_HEIGHT
	sel.target = VIDEO_SEL_TGT_CROP;
	sel.rect.left = CONFIG_VIDEO_SOURCE_CROP_LEFT;
	sel.rect.top = CONFIG_VIDEO_SOURCE_CROP_TOP;
	sel.rect.width = CONFIG_VIDEO_SOURCE_CROP_WIDTH;
	sel.rect.height = CONFIG_VIDEO_SOURCE_CROP_HEIGHT;
	if (video_set_selection(video_dev, &sel)) {
		LOG_ERR("Unable to set selection crop");
		return 0;
	}
	LOG_INF("Selection crop set to (%u,%u)/%ux%u",
		sel.rect.left, sel.rect.top, sel.rect.width, sel.rect.height);
#endif

	/* Set format */
	fmt.width = CONFIG_VIDEO_WIDTH;
	fmt.height = CONFIG_VIDEO_HEIGHT;
	fmt.pixelformat = VIDEO_PIX_FMT_RGB565;

	/*
	 * Check (if possible) if targeted size is same as crop
	 * and if compose is necessary
	 */
	sel.target = VIDEO_SEL_TGT_CROP;
	err = video_get_selection(video_dev, &sel);
	if (err < 0 && err != -ENOSYS) {
		LOG_ERR("Unable to get selection crop");
		return 0;
	}

	if (err == 0 && (sel.rect.width != fmt.width || sel.rect.height != fmt.height)) {
		sel.target = VIDEO_SEL_TGT_COMPOSE;
		sel.rect.left = 0;
		sel.rect.top = 0;
		sel.rect.width = fmt.width;
		sel.rect.height = fmt.height;
		err = video_set_selection(video_dev, &sel);
		if (err < 0 && err != -ENOSYS) {
			LOG_ERR("Unable to set selection compose");
			return 0;
		}
	}

	if (video_set_format(video_dev, &fmt)) {
		LOG_ERR("Unable to set up video format");
		return 0;
	}

	LOG_INF("- Format: %c%c%c%c %ux%u %u", (char)fmt.pixelformat, (char)(fmt.pixelformat >> 8),
		(char)(fmt.pixelformat >> 16), (char)(fmt.pixelformat >> 24), fmt.width, fmt.height,
		fmt.pitch);

	/* Alloc video buffers and enqueue for capture */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		buffers[i] = video_buffer_aligned_alloc(fmt.size, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
							K_NO_WAIT);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return 0;
		}
		buffers[i]->type = type;
		video_enqueue(video_dev, buffers[i]);
	}

	/* Set controls */
	struct video_control ctrl = {.id = VIDEO_CID_HFLIP, .val = 1};

	if (IS_ENABLED(CONFIG_VIDEO_HFLIP)) {
		video_set_ctrl(video_dev, &ctrl);
	}

	if (IS_ENABLED(CONFIG_VIDEO_VFLIP)) {
		ctrl.id = VIDEO_CID_VFLIP;
		video_set_ctrl(video_dev, &ctrl);
	}

	/* Start video capture */
	if (video_stream_start(video_dev, type)) {
		LOG_ERR("Unable to start capture (interface)");
		return 0;
	}

	display_blanking_off(display_dev);

	const lv_img_dsc_t video_img = {
		.header.w = CONFIG_VIDEO_WIDTH,
		.header.h = CONFIG_VIDEO_HEIGHT,
		.data_size = CONFIG_VIDEO_WIDTH * CONFIG_VIDEO_HEIGHT * sizeof(lv_color_t),
		.header.cf = LV_COLOR_FORMAT_NATIVE,
		.data = (const uint8_t *)buffers[0]->buffer,
	};

	lv_obj_t *screen = lv_img_create(lv_scr_act());

	LOG_INF("- Capture started");

	/* Grab video frames */
	vbuf->type = type;
	while (1) {
		err = video_dequeue(video_dev, &vbuf, K_FOREVER);
		if (err) {
			LOG_ERR("Unable to dequeue video buf");
			return 0;
		}

		lv_img_set_src(screen, &video_img);
		lv_obj_align(screen, LV_ALIGN_BOTTOM_LEFT, 0, 0);

		lv_task_handler();

		err = video_enqueue(video_dev, vbuf);
		if (err) {
			LOG_ERR("Unable to requeue video buf");
			return 0;
		}
	}
}

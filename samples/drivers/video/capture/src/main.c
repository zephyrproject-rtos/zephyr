/*
 * Copyright (c) 2019 Linaro Limited
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#if !DT_HAS_CHOSEN(zephyr_camera)
#error No camera chosen in devicetree. Missing "--shield" or "--snippet video-sw-generator" flag?
#endif

static inline int app_setup_display(const struct device *const display_dev, const uint32_t pixfmt)
{
	struct display_capabilities capabilities;
	int ret = 0;

	LOG_INF("Display device: %s", display_dev->name);

	if (!device_is_ready(display_dev)) {
		LOG_ERR("%s: display device not ready.", display_dev->name);
		return -ENOSYS;
	}

	display_get_capabilities(display_dev, &capabilities);

	LOG_INF("- Capabilities:");
	LOG_INF("  x_resolution = %u, y_resolution = %u, supported_pixel_formats = %u"
		"  current_pixel_format = %u, current_orientation = %u",
		capabilities.x_resolution, capabilities.y_resolution,
		capabilities.supported_pixel_formats, capabilities.current_pixel_format,
		capabilities.current_orientation);

	/* Set display pixel format to match the one in use by the camera */
	switch (pixfmt) {
	case VIDEO_PIX_FMT_RGB565:
		if (capabilities.current_pixel_format != PIXEL_FORMAT_RGB_565) {
			ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_RGB_565);
		}
		break;
	case VIDEO_PIX_FMT_XRGB32:
		if (capabilities.current_pixel_format != PIXEL_FORMAT_ARGB_8888) {
			ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_ARGB_8888);
		}
		break;
	default:
		LOG_ERR("Display pixel format not supported by this sample");
		return -ENOTSUP;
	}
	if (ret < 0) {
		LOG_ERR("Unable to set display format");
		return ret;
	}

	/* Turn off blanking if driver supports it */
	ret = display_blanking_off(display_dev);
	if (ret == -ENOSYS) {
		LOG_DBG("Display blanking off not available");
		ret = 0;
	}

	return ret;
}

static int app_display_frame(const struct device *const display_dev,
			     const struct video_buffer *const vbuf,
			     const struct video_format *const fmt)
{
	struct display_buffer_descriptor buf_desc = {
		.buf_size = vbuf->bytesused,
		.width = fmt->width,
		.pitch = buf_desc.width,
		.height = vbuf->bytesused / fmt->pitch,
	};

	return display_write(display_dev, 0, vbuf->line_offset, &buf_desc, vbuf->buffer);
}

static int app_setup_video_selection(const struct device *const video_dev,
				     const struct video_format *const fmt)
{
	struct video_selection sel = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};
	int ret;

	/* Set the crop setting only if configured */
	if (CONFIG_VIDEO_SOURCE_CROP_WIDTH > 0) {
		sel.target = VIDEO_SEL_TGT_CROP;
		sel.rect.left = CONFIG_VIDEO_SOURCE_CROP_LEFT;
		sel.rect.top = CONFIG_VIDEO_SOURCE_CROP_TOP;
		sel.rect.width = CONFIG_VIDEO_SOURCE_CROP_WIDTH;
		sel.rect.height = CONFIG_VIDEO_SOURCE_CROP_HEIGHT;

		ret = video_set_selection(video_dev, &sel);
		if (ret < 0) {
			LOG_ERR("Unable to set selection crop");
			return ret;
		}

		LOG_INF("Crop window set to (%u,%u)/%ux%u",
			sel.rect.left, sel.rect.top, sel.rect.width, sel.rect.height);
	}

	/*
	 * Check (if possible) if targeted size is same as crop
	 * and if compose is necessary
	 */
	sel.target = VIDEO_SEL_TGT_CROP;
	ret = video_get_selection(video_dev, &sel);
	if (ret < 0 && ret != -ENOSYS) {
		LOG_ERR("Unable to get selection crop");
		return ret;
	}

	if (ret == 0 && (sel.rect.width != fmt->width || sel.rect.height != fmt->height)) {
		sel.target = VIDEO_SEL_TGT_COMPOSE;
		sel.rect.left = 0;
		sel.rect.top = 0;
		sel.rect.width = fmt->width;
		sel.rect.height = fmt->height;

		ret = video_set_selection(video_dev, &sel);
		if (ret < 0 && ret != -ENOSYS) {
			LOG_ERR("Unable to set selection compose");
			return ret;
		}

		LOG_INF("Compose window set to (%u,%u)/%ux%u",
			sel.rect.left, sel.rect.top, sel.rect.width, sel.rect.height);
	}

	return 0;
}

static int app_query_video_info(const struct device *const video_dev,
				struct video_caps *const caps,
				struct video_format *const fmt)
{
	int ret;

	LOG_INF("Video device: %s", video_dev->name);

	if (!device_is_ready(video_dev)) {
		LOG_ERR("%s: video device is not ready", video_dev->name);
		return -ENOSYS;
	}

	/* Get capabilities */
	ret = video_get_caps(video_dev, caps);
	if (ret < 0) {
		LOG_ERR("Unable to retrieve video capabilities");
		return ret;
	}

	LOG_INF("- Capabilities:");
	for (int i = 0; caps->format_caps[i].pixelformat; i++) {
		const struct video_format_cap *fcap = &caps->format_caps[i];

		LOG_INF("  %s width [%u; %u; %u] height [%u; %u; %u]",
			VIDEO_FOURCC_TO_STR(fcap->pixelformat),
			fcap->width_min, fcap->width_max, fcap->width_step,
			fcap->height_min, fcap->height_max, fcap->height_step);
	}

	/* Get default/native format */
	ret = video_get_format(video_dev, fmt);
	if (ret < 0) {
		LOG_ERR("Unable to retrieve video format");
	}

	/* Adjust video format according to the configuration */
	if (CONFIG_VIDEO_FRAME_HEIGHT > 0) {
		fmt->height = CONFIG_VIDEO_FRAME_HEIGHT;
	}
	if (CONFIG_VIDEO_FRAME_WIDTH > 0) {
		fmt->width = CONFIG_VIDEO_FRAME_WIDTH;
	}
	if (strcmp(CONFIG_VIDEO_PIXEL_FORMAT, "") != 0) {
		fmt->pixelformat = VIDEO_FOURCC_FROM_STR(CONFIG_VIDEO_PIXEL_FORMAT);
	}

	return 0;
}

static int app_setup_video_format(const struct device *const video_dev,
				  struct video_format *const fmt)
{
	int ret;

	LOG_INF("- Video format: %s %ux%u",
		VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);

	ret = video_set_compose_format(video_dev, fmt);
	if (ret < 0) {
		LOG_ERR("Unable to set format");
		return ret;
	}

	return 0;
}

static int app_setup_video_frmival(const struct device *const video_dev,
				   struct video_format *const fmt)
{
	struct video_frmival frmival = {};
	struct video_frmival_enum fie = {
		.format = fmt,
	};
	int ret;

	LOG_INF("- Supported frame intervals for the default format:");

	while (video_enum_frmival(video_dev, &fie) == 0) {
		if (fie.type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
			LOG_INF("   %u/%u", fie.discrete.numerator, fie.discrete.denominator);
		} else {
			LOG_INF("   [min = %u/%u; max = %u/%u; step = %u/%u]",
				fie.stepwise.min.numerator, fie.stepwise.min.denominator,
				fie.stepwise.max.numerator, fie.stepwise.max.denominator,
				fie.stepwise.step.numerator, fie.stepwise.step.denominator);
		}
		fie.index++;
	}

	ret = video_get_frmival(video_dev, &frmival);
	if (ret == -ENOTSUP || ret == -ENOSYS) {
		LOG_WRN("The video source does not support frame rate control");
	} else if (ret < 0) {
		LOG_ERR("Error while getting the frame interval");
		return ret;
	} else if (ret == 0) {
		LOG_INF("- Default frame rate : %f fps",
			1.0 * frmival.denominator / frmival.numerator);
	}

	return 0;
}

static int app_setup_video_controls(const struct device *const video_dev)
{
	int ret;

	/* Get supported controls */
	LOG_INF("- Supported controls:");
	const struct device *last_dev = NULL;
	struct video_ctrl_query cq = {.dev = video_dev, .id = VIDEO_CTRL_FLAG_NEXT_CTRL};

	while (video_query_ctrl(&cq) == 0) {
		if (cq.dev != last_dev) {
			last_dev = cq.dev;
			LOG_INF("\t\tdevice: %s", cq.dev->name);
		}
		video_print_ctrl(&cq);
		cq.id |= VIDEO_CTRL_FLAG_NEXT_CTRL;
	}

	/* Set controls */
	struct video_control ctrl = {.id = VIDEO_CID_HFLIP, .val = 1};

	if (IS_ENABLED(CONFIG_VIDEO_CTRL_HFLIP)) {
		ret = video_set_ctrl(video_dev, &ctrl);
		if (ret < 0) {
			LOG_ERR("Failed to set horizontal flip");
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_VIDEO_CTRL_VFLIP)) {
		ctrl.id = VIDEO_CID_VFLIP;
		ret = video_set_ctrl(video_dev, &ctrl);
		if (ret < 0) {
			LOG_ERR("Failed to set vertical flip");
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_TEST)) {
		ctrl.id = VIDEO_CID_TEST_PATTERN;
		ret = video_set_ctrl(video_dev, &ctrl);
		if (ret < 0 && ret != -ENOTSUP) {
			LOG_WRN("Failed to set the test pattern");
		}
	}

	return 0;
}

static int app_setup_video_buffers(const struct device *const video_dev,
				   struct video_caps *const caps,
				   struct video_format *const fmt)
{
	int ret;
	struct video_buffer vbuf = {.type = VIDEO_BUF_TYPE_OUTPUT};

	/* Request buffers */
	struct video_buffer_request vbr = {
		.count = CONFIG_VIDEO_BUFFER_POOL_NUM_MAX,
		.size = fmt->size,
		.timeout = K_NO_WAIT,
	};

	ret = video_request_buffers(&vbr);
	if (ret || vbr.count < caps->min_vbuf_count) {
		LOG_ERR("Unable to request buffers or not enough buffers to start streaming");
		return 0;
	}

	/* Enqueue buffers */
	for (uint8_t i = 0; i < vbr.count; i++) {
		vbuf.index = vbr.start_index++;
		video_enqueue(video_dev, &vbuf);
	}

	return 0;
}

int main(void)
{
	const struct device *const video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));
	const struct device *const display_dev = DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_display));
	struct video_buffer *vbuf = &(struct video_buffer){};
	struct video_format fmt = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};
	struct video_caps caps = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};
	unsigned int frame = 0;
	int ret;

	/* When the video shell is enabled, do not run the capture loop */
	if (IS_ENABLED(CONFIG_VIDEO_SHELL)) {
		LOG_INF("Letting the user control the device with the video shell");
		return 0;
	}

	ret = app_query_video_info(video_dev, &caps, &fmt);
	if (ret < 0) {
		goto err;
	}

	ret = app_setup_video_selection(video_dev, &fmt);
	if (ret < 0) {
		goto err;
	}

	ret = app_setup_video_format(video_dev, &fmt);
	if (ret < 0) {
		goto err;
	}

	ret = app_setup_video_frmival(video_dev, &fmt);
	if (ret < 0) {
		goto err;
	}

	ret = app_setup_video_controls(video_dev);
	if (ret < 0) {
		goto err;
	}

	if (DT_HAS_CHOSEN(zephyr_display)) {
		ret = app_setup_display(display_dev, fmt.pixelformat);
		if (ret < 0) {
			goto err;
		}
	}

	ret = app_setup_video_buffers(video_dev, &caps, &fmt);
	if (ret < 0) {
		goto err;
	}

	ret = video_stream_start(video_dev, VIDEO_BUF_TYPE_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Unable to start capture (interface)");
		goto err;
	}

	LOG_INF("Capture started");

	vbuf->type = VIDEO_BUF_TYPE_OUTPUT;
	while (1) {
		ret = video_dequeue(video_dev, &vbuf, K_FOREVER);
		if (ret < 0) {
			LOG_ERR("Unable to dequeue video buf");
			goto err;
		}

		LOG_INF("Got frame %u! size: %u; timestamp %u ms",
			frame++, vbuf->bytesused, vbuf->timestamp);

		if (DT_HAS_CHOSEN(zephyr_display)) {
			ret = app_display_frame(display_dev, vbuf, &fmt);
			if (ret != 0) {
				LOG_WRN("Failed to display this frame");
			}
		}

		ret = video_enqueue(video_dev, vbuf);
		if (ret < 0) {
			LOG_ERR("Unable to requeue video buf");
			goto err;
		}
	}

err:
	LOG_ERR("Aborting sample");
	return 0;
}

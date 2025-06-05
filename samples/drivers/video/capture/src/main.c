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

#ifdef CONFIG_TEST
#include "check_test_pattern.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);
#else
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);
#endif

#if DT_HAS_CHOSEN(zephyr_display)
static inline int display_setup(const struct device *const display_dev, const uint32_t pixfmt)
{
	struct display_capabilities capabilities;
	int ret = 0;

	LOG_INF("Display device: %s", display_dev->name);

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
		if (capabilities.current_pixel_format != PIXEL_FORMAT_BGR_565) {
			ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_BGR_565);
		}
		break;
	case VIDEO_PIX_FMT_XRGB32:
		if (capabilities.current_pixel_format != PIXEL_FORMAT_ARGB_8888) {
			ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_ARGB_8888);
		}
		break;
	default:
		return -ENOTSUP;
	}

	if (ret) {
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

static inline void video_display_frame(const struct device *const display_dev,
				       const struct video_buffer *const vbuf,
				       const struct video_format fmt)
{
	struct display_buffer_descriptor buf_desc = {
		.buf_size = vbuf->bytesused,
		.width = fmt.width,
		.pitch = buf_desc.width,
		.height = vbuf->bytesused / fmt.pitch,
	};

	display_write(display_dev, 0, vbuf->line_offset, &buf_desc, vbuf->buffer);
}
#endif

int main(void)
{
	struct video_buffer *buffers[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];
	struct video_buffer *vbuf = &(struct video_buffer){};
	const struct device *video_dev;
	struct video_format fmt;
	struct video_caps caps;
	struct video_frmival frmival;
	struct video_frmival_enum fie;
	enum video_buf_type type = VIDEO_BUF_TYPE_OUTPUT;
	unsigned int frame = 0;
	size_t bsize;
	int i = 0;
	int err;

	/* When the video shell is enabled, do not run the capture loop */
	if (IS_ENABLED(CONFIG_VIDEO_SHELL)) {
		LOG_INF("Letting the user control the device with the video shell");
		return 0;
	}

	video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));
	if (!device_is_ready(video_dev)) {
		LOG_ERR("%s: video device is not ready", video_dev->name);
		return 0;
	}

	LOG_INF("Video device: %s", video_dev->name);

	/* Get capabilities */
	caps.type = type;
	if (video_get_caps(video_dev, &caps)) {
		LOG_ERR("Unable to retrieve video capabilities");
		return 0;
	}

	LOG_INF("- Capabilities:");
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		/* fourcc to string */
		LOG_INF("  %s width [%u; %u; %u] height [%u; %u; %u]",
			VIDEO_FOURCC_TO_STR(fcap->pixelformat),
			fcap->width_min, fcap->width_max, fcap->width_step,
			fcap->height_min, fcap->height_max, fcap->height_step);
		i++;
	}

	/* Get default/native format */
	fmt.type = type;
	if (video_get_format(video_dev, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return 0;
	}

#if CONFIG_VIDEO_FRAME_HEIGHT
	fmt.height = CONFIG_VIDEO_FRAME_HEIGHT;
#endif

#if CONFIG_VIDEO_FRAME_WIDTH
	fmt.width = CONFIG_VIDEO_FRAME_WIDTH;
#endif

	if (strcmp(CONFIG_VIDEO_PIXEL_FORMAT, "")) {
		fmt.pixelformat = VIDEO_FOURCC_FROM_STR(CONFIG_VIDEO_PIXEL_FORMAT);
	}

	LOG_INF("- Video format: %s %ux%u",
		VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height);

	if (video_set_format(video_dev, &fmt)) {
		LOG_ERR("Unable to set format");
		return 0;
	}

	if (!video_get_frmival(video_dev, &frmival)) {
		LOG_INF("- Default frame rate : %f fps",
			1.0 * frmival.denominator / frmival.numerator);
	}

	LOG_INF("- Supported frame intervals for the default format:");
	memset(&fie, 0, sizeof(fie));
	fie.format = &fmt;
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

	/* Get supported controls */
	LOG_INF("- Supported controls:");

	struct video_ctrl_query cq = {.id = VIDEO_CTRL_FLAG_NEXT_CTRL};

	while (!video_query_ctrl(video_dev, &cq)) {
		video_print_ctrl(video_dev, &cq);
		cq.id |= VIDEO_CTRL_FLAG_NEXT_CTRL;
	}

	/* Set controls */
	struct video_control ctrl = {.id = VIDEO_CID_HFLIP, .val = 1};

	if (IS_ENABLED(CONFIG_VIDEO_CTRL_HFLIP)) {
		video_set_ctrl(video_dev, &ctrl);
	}

	if (IS_ENABLED(CONFIG_TEST)) {
		ctrl.id = VIDEO_CID_TEST_PATTERN;
		video_set_ctrl(video_dev, &ctrl);
	}

#if DT_HAS_CHOSEN(zephyr_display)
	const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev)) {
		LOG_ERR("%s: display device not ready.", display_dev->name);
		return 0;
	}

	err = display_setup(display_dev, fmt.pixelformat);
	if (err) {
		LOG_ERR("Unable to set up display");
		return err;
	}
#endif

	/* Size to allocate for each buffer */
	if (caps.min_line_count == LINE_COUNT_HEIGHT) {
		bsize = fmt.pitch * fmt.height;
	} else {
		bsize = fmt.pitch * caps.min_line_count;
	}

	/* Alloc video buffers and enqueue for capture */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		/*
		 * For some hardwares, such as the PxP used on i.MX RT1170 to do image rotation,
		 * buffer alignment is needed in order to achieve the best performance
		 */
		buffers[i] = video_buffer_aligned_alloc(bsize, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
							K_FOREVER);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return 0;
		}
		buffers[i]->type = type;
		video_enqueue(video_dev, buffers[i]);
	}

	/* Start video capture */
	if (video_stream_start(video_dev, type)) {
		LOG_ERR("Unable to start capture (interface)");
		return 0;
	}

	LOG_INF("Capture started");

	/* Grab video frames */
	vbuf->type = type;
	while (1) {
		err = video_dequeue(video_dev, &vbuf, K_FOREVER);
		if (err) {
			LOG_ERR("Unable to dequeue video buf");
			return 0;
		}

		LOG_DBG("Got frame %u! size: %u; timestamp %u ms",
			frame++, vbuf->bytesused, vbuf->timestamp);

#ifdef CONFIG_TEST
		if (is_colorbar_ok(vbuf->buffer, fmt)) {
			LOG_DBG("Pattern OK!\n");
		}
#endif

#if DT_HAS_CHOSEN(zephyr_display)
		video_display_frame(display_dev, vbuf, fmt);
#endif

		err = video_enqueue(video_dev, vbuf);
		if (err) {
			LOG_ERR("Unable to requeue video buf");
			return 0;
		}
	}
}

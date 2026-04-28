/*
 * Copyright 2025 - 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbh.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

/* Wait for video device connection */
static void wait_for_video_connection(const struct device *uvc_dev, struct video_format *fmt,
				     enum video_buf_type type)
{
	int ret;

	while (true) {
		fmt->type = type;
		ret = video_get_format(uvc_dev, fmt);
		if (ret == 0) {
			LOG_INF("Video device connected!");
			return;
		}
		k_sleep(K_MSEC(10));
	}
}

/* Log video capabilities, frame intervals and supported controls */
static void log_video_info(const struct device *uvc_dev,
			   const struct video_caps *const caps,
			   const struct video_format *const fmt)
{
	struct video_ctrl_query cq = {.dev = uvc_dev, .id = VIDEO_CTRL_FLAG_NEXT_CTRL};
	const struct device *last_dev = NULL;
	struct video_frmival_enum fie;

	LOG_INF("- Capabilities:");
	for (uint8_t i = 0; caps->format_caps[i].pixelformat; i++) {
		LOG_INF("  %s width [%u; %u; %u] height [%u; %u; %u]",
			VIDEO_FOURCC_TO_STR(caps->format_caps[i].pixelformat),
			caps->format_caps[i].width_min, caps->format_caps[i].width_max,
			caps->format_caps[i].width_step, caps->format_caps[i].height_min,
			caps->format_caps[i].height_max, caps->format_caps[i].height_step);
	}

	LOG_INF("- Supported frame intervals for the default format:");
	memset(&fie, 0, sizeof(fie));
	fie.format = fmt;

	while (video_enum_frmival(uvc_dev, &fie) == 0) {
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

	LOG_INF("- Supported controls:");
	while (video_query_ctrl(&cq) == 0) {
		if (cq.dev != last_dev) {
			last_dev = cq.dev;
			LOG_INF("\t\tdevice: %s", cq.dev->name);
		}

		video_print_ctrl(&cq);
		cq.id |= VIDEO_CTRL_FLAG_NEXT_CTRL;
	}
}

/* Configure video format and frame rate */
static int configure_video_format_and_rate(const struct device *uvc_dev,
					   struct video_format *const fmt)
{
	struct video_frmival frmival;
	int ret;

	fmt->pixelformat = VIDEO_FOURCC_FROM_STR(CONFIG_APP_VIDEO_PIXEL_FORMAT);
	fmt->width = CONFIG_APP_VIDEO_FRAME_WIDTH;
	fmt->height = CONFIG_APP_VIDEO_FRAME_HEIGHT;

	LOG_INF("- Expected video format: %s %ux%u",
			VIDEO_FOURCC_TO_STR(fmt->pixelformat), fmt->width, fmt->height);

	ret = video_set_format(uvc_dev, fmt);
	if (ret != 0) {
		LOG_ERR("Unable to set format");
		return ret;
	}

	ret = video_get_frmival(uvc_dev, &frmival);
	if (ret == 0) {
		LOG_INF("- Default frame rate : %f fps",
			1.0 * frmival.denominator / frmival.numerator);
	}

	if (CONFIG_APP_VIDEO_TARGET_FPS > 0) {
		frmival.denominator = CONFIG_APP_VIDEO_TARGET_FPS;
		frmival.numerator = 1;
		if (video_set_frmival(uvc_dev, &frmival) == 0 &&
			video_get_frmival(uvc_dev, &frmival) == 0) {
			LOG_INF("- Target frame rate set to: %f fps",
				1.0 * frmival.denominator / frmival.numerator);
		} else {
			LOG_WRN("- Unable to set target frame rate to %u fps",
				CONFIG_APP_VIDEO_TARGET_FPS);
		}
	}

	return 0;
}

static void cleanup_video_bufs(struct video_buffer *allocated_vbufs[],
			       uint8_t *const allocated_count)
{
	for (int i = 0; i < *allocated_count; i++) {
		if (allocated_vbufs[i]) {
			video_buffer_release(allocated_vbufs[i]);
			allocated_vbufs[i] = NULL;
		}
	}

	*allocated_count = 0;
}

/* Allocate video buffers and start streaming */
static int allocate_buffers_and_start_stream(const struct device *uvc_dev,
					     struct video_buffer *allocated_vbufs[],
					     uint8_t *const allocated_count,
					     const size_t bsize,
					     const enum video_buf_type type)
{
	struct video_buffer *vbuf = NULL;
	uint8_t i = 0;
	int ret;

	LOG_INF("Allocating %d video buffers, size=%zu", CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, bsize);

	for (i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
		vbuf = video_buffer_aligned_alloc(bsize, CONFIG_VIDEO_BUFFER_POOL_ALIGN, K_FOREVER);
		if (vbuf == NULL) {
			LOG_ERR("Unable to alloc video buffer %d/%d", i,
				CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
			ret = -ENOMEM;
			goto exit_free;
		}

		allocated_vbufs[i] = vbuf;
		*allocated_count = i + 1;
		vbuf->type = type;
	}

	if (*allocated_count < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX) {
		LOG_WRN("Only allocated %u/%u buffers!",
			*allocated_count, CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
	}

	for (i = 0; i < *allocated_count; i++) {
		video_enqueue(uvc_dev, allocated_vbufs[i]);
	}

	ret = video_stream_start(uvc_dev, type);
	if (ret != 0) {
		LOG_ERR("Unable to start capture (interface)");
		goto exit_free;
	}

	LOG_INF("Capture started");
	return 0;

exit_free:
	cleanup_video_bufs(allocated_vbufs, allocated_count);
	return ret;
}

/* Initialize and start video streaming */
static int setup_video_streaming(const struct device *uvc_dev,
				 struct video_buffer *allocated_vbufs[],
				 uint8_t *const allocated_count,
				 struct video_format *const fmt)
{
	struct video_caps caps = {.type = VIDEO_BUF_TYPE_OUTPUT};
	size_t bsize;
	int ret;

	/* Get capabilities */
	ret = video_get_caps(uvc_dev, &caps);
	if (ret != 0) {
		LOG_ERR("Unable to retrieve video capabilities");
		return ret;
	}

	ret = configure_video_format_and_rate(uvc_dev, fmt);
	if (ret != 0) {
		return ret;
	}

	log_video_info(uvc_dev, &caps, fmt);

	bsize = fmt->width * fmt->height * 2;

	if (caps.min_vbuf_count > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX ||
	    bsize > (CONFIG_APP_VIDEO_FRAME_WIDTH * CONFIG_APP_VIDEO_FRAME_HEIGHT * 2)) {
		LOG_ERR("Not enough buffers or memory to start streaming");
		return -ENOMEM;
	}

	ret = allocate_buffers_and_start_stream(uvc_dev, allocated_vbufs,
						allocated_count, bsize, caps.type);

	return ret;
}

/* Cleanup video streaming resources */
static int cleanup_video_streaming(const struct device *uvc_dev,
				    struct video_buffer *allocated_vbufs[],
				    uint8_t *const allocated_count,
				    const enum video_buf_type type)
{
	int ret;

	LOG_INF("Cleaning up video streaming resources...");

	/* -ENODEV is expected when device is disconnected, continue cleanup */
	ret = video_stream_stop(uvc_dev, type);
	if (ret != 0 && ret != -ENODEV) {
		LOG_ERR("Failed to stop video stream: %d", ret);
		return ret;
	}

	cleanup_video_bufs(allocated_vbufs, allocated_count);
	LOG_INF("Video streaming cleanup completed");

	return 0;
}

static int process_video_stream(const struct device *uvc_dev, uint32_t *frame_count)
{
	struct video_buffer *vbuf = NULL;
	int err;

	err = video_dequeue(uvc_dev, &vbuf, K_FOREVER);
	if (err != 0) {
		if (err == -ENODEV) {
			LOG_WRN("Video device disconnected");
			return -ENODEV;
		}
		LOG_ERR("Unable to dequeue video buf: %d", err);
		return 0;
	}

	(*frame_count)++;
	if (*frame_count % 100 == 0) {
		LOG_INF("Received %u frames", *frame_count);
	}

	err = video_enqueue(uvc_dev, vbuf);
	if (err == -ENODEV) {
		LOG_WRN("Video device disconnected during enqueue");
		return -ENODEV;
	} else if (err != 0) {
		LOG_ERR("Unable to requeue video buf: %d", err);
	}

	return 0;
}

int main(void)
{
	const struct device *uvc_dev = device_get_binding("usbh_uvc_0");
	struct video_buffer *allocated_vbufs[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX] = {NULL};
	struct video_format fmt;
	struct k_poll_signal sig;
	struct k_poll_event evt[1];
	k_timeout_t timeout = K_FOREVER;
	enum video_buf_type type = VIDEO_BUF_TYPE_OUTPUT;
	uint32_t frame_count = 0;
	uint8_t allocated_count = 0;
	int err;

	if (!device_is_ready(uvc_dev)) {
		LOG_ERR("USB host video device %s is not ready", uvc_dev->name);
		return 0;
	}

	err = usbh_init(&uhs_ctx);
	if (err) {
		LOG_ERR("Failed to initialize host support");
		return err;
	}

	err = usbh_enable(&uhs_ctx);
	if (err) {
		LOG_ERR("Failed to enable USB host support");
		return err;
	}

	LOG_INF("USB host video device %s is ready", uvc_dev->name);

	k_poll_signal_init(&sig);
	k_poll_event_init(&evt[0], K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig);

	err = video_set_signal(uvc_dev, &sig);
	if (err != 0) {
		LOG_WRN("Failed to setup the signal on %s output endpoint", uvc_dev->name);
		timeout = K_MSEC(10);
	}

	while (true) {
		/* Wait for video device connection */
		wait_for_video_connection(uvc_dev, &fmt, type);

		/* Setup and start streaming */
		err = setup_video_streaming(uvc_dev, allocated_vbufs, &allocated_count, &fmt);
		if (err != 0) {
			LOG_ERR("Failed to setup video streaming");
			k_sleep(K_MSEC(1000));
			continue;
		}

		/* Reset frame counter for new connection */
		frame_count = 0;

		/* Main video streaming loop - process frames until disconnect */
		while (true) {
			err = k_poll(evt, ARRAY_SIZE(evt), timeout);
			if (err != 0 && err != -EAGAIN) {
				LOG_WRN("Poll failed with error %d", err);
				continue;
			}

			err = process_video_stream(uvc_dev, &frame_count);
			if (err == -ENODEV) {
				break;
			}

			k_poll_signal_reset(&sig);
		}

		err = cleanup_video_streaming(uvc_dev, allocated_vbufs, &allocated_count, type);
		if (err != 0) {
			LOG_ERR("Failed to cleanup video streaming: %d", err);
		}

		LOG_INF("Video device disconnected, waiting for reconnection...");
	}

	return 0;
}

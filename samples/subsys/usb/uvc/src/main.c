/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>

#include <sample_usbd.h>

#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usbd_uvc.h>

LOG_MODULE_REGISTER(uvc_sample, LOG_LEVEL_INF);

const static struct device *const uvc_dev = DEVICE_DT_GET(DT_NODELABEL(uvc));
const static struct device *const video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));

/* Format capabilities of video_dev, used everywhere through the sample */
static struct video_caps video_caps = {.type = VIDEO_BUF_TYPE_OUTPUT};

static bool app_is_standard_format(uint32_t pixfmt)
{
	return pixfmt == VIDEO_PIX_FMT_GREY || pixfmt == VIDEO_PIX_FMT_JPEG ||
	       pixfmt == VIDEO_PIX_FMT_YUYV;
}

/* Check whether the video device supports one of the wisespread image sensor formats */
static bool app_has_standard_formats(void)
{
	const struct video_format_cap *fmts = video_caps.format_caps;

	for (int i = 0; fmts[i].pixelformat != 0; i++) {
		if (app_is_standard_format(fmts[i].pixelformat)) {
			return true;
		}
	}

	return false;
}

static void app_add_format(uint32_t pixfmt, uint16_t width, uint16_t height, bool has_std_fmts)
{
	struct video_format fmt = {
		.pixelformat = pixfmt,
		.width = width,
		.height = height,
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};
	int ret;

	/* If the system has any standard pixel format, only propose them to the host */
	if (has_std_fmts && !app_is_standard_format(pixfmt)) {
		return;
	}

	/* Set the format to get the pitch */
	ret = video_set_compose_format(video_dev, &fmt);
	if (ret != 0) {
		LOG_ERR("Could not set the format of %s", video_dev->name);
		return;
	}

	if (fmt.size > CONFIG_VIDEO_BUFFER_POOL_SZ_MAX) {
		LOG_WRN("Skipping format %ux%u", fmt.width, fmt.height);
		return;
	}

	uvc_add_format(uvc_dev, &fmt);
}

struct video_resolution {
	uint16_t width;
	uint16_t height;
};

static struct video_resolution video_common_fmts[] = {
	{ .width = 3840,	.height = 2160,	},	/* UHD */
	{ .width = 1920,	.height = 1080,	},	/* FHD */
	{ .width = 1280,	.height = 1024,	},	/* SXGA */
	{ .width = 1280,	.height = 720,	},	/* HD */
	{ .width = 800,		.height = 600,	},	/* SVGA */
	{ .width = 854,		.height = 480,	},	/* WVGA */
	{ .width = 640,		.height = 480,	},	/* VGA */
	{ .width = 320,		.height = 240,	},	/* QVGA */
	{ .width = 160,		.height = 120,	},	/* QQVGA */
};

/* Submit to UVC only the formats expected to be working (enough memory for the size, etc.) */
static void app_add_filtered_formats(void)
{
	const bool has_std_fmts = app_has_standard_formats();

	for (int i = 0; video_caps.format_caps[i].pixelformat != 0; i++) {
		const struct video_format_cap *vcap = &video_caps.format_caps[i];
		int range_count = 0;

		app_add_format(vcap->pixelformat, vcap->width_min, vcap->height_min, has_std_fmts);

		if (vcap->width_min != vcap->width_max || vcap->height_min != vcap->height_max) {
			app_add_format(vcap->pixelformat, vcap->width_max, vcap->height_max,
				       has_std_fmts);
		}

		if (vcap->width_step == 0 && vcap->height_step == 0) {
			continue;
		}

		/* RANGE Resolution processing */
		for (int j = 0; j < ARRAY_SIZE(video_common_fmts); j++) {
			if (range_count >= CONFIG_VIDEO_MAX_RANGE_RESOLUTIONS) {
				break;
			}
			if (!IN_RANGE(video_common_fmts[j].width,
				      vcap->width_min, vcap->width_max) ||
			    !IN_RANGE(video_common_fmts[j].height,
				      vcap->height_min, vcap->height_max)) {
				continue;
			}
			if ((video_common_fmts[j].width - vcap->width_min) % vcap->width_step ||
			    (video_common_fmts[j].height - vcap->height_min) % vcap->height_step) {
				continue;
			}

			app_add_format(vcap->pixelformat, video_common_fmts[j].width,
				       video_common_fmts[j].height, has_std_fmts);
		}
	}
}

int main(void)
{
	struct usbd_context *sample_usbd;
	struct video_buffer *vbuf;
	struct video_format fmt = {0};
	struct video_frmival frmival = {0};
	struct k_poll_signal sig;
	struct k_poll_event evt[1];
	k_timeout_t timeout = K_FOREVER;
	int ret;

	if (!device_is_ready(video_dev)) {
		LOG_ERR("video source %s failed to initialize", video_dev->name);
		return -ENODEV;
	}

	ret = video_get_caps(video_dev, &video_caps);
	if (ret != 0) {
		LOG_ERR("Unable to retrieve video capabilities");
		return 0;
	}

	/* Must be called before usb_enable() */
	uvc_set_video_dev(uvc_dev, video_dev);

	/* Must be called before uvc_set_video_dev() */
	app_add_filtered_formats();

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("Waiting the host to select the video format");

	while (true) {
		fmt.type = VIDEO_BUF_TYPE_INPUT;

		ret = video_get_format(uvc_dev, &fmt);
		if (ret == 0) {
			break;
		}
		if (ret != -EAGAIN) {
			LOG_ERR("Failed to get the video format");
			return ret;
		}

		k_sleep(K_MSEC(10));
	}

	ret = video_get_frmival(uvc_dev, &frmival);
	if (ret != 0) {
		LOG_ERR("Failed to get the video frame interval");
		return ret;
	}

	LOG_INF("The host selected format '%s' %ux%u at frame interval %u/%u",
		VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height,
		frmival.numerator, frmival.denominator);

	fmt.type = VIDEO_BUF_TYPE_OUTPUT;

	ret = video_set_compose_format(video_dev, &fmt);
	if (ret != 0) {
		LOG_WRN("Could not set the format of %s", video_dev->name);
	}

	ret = video_set_frmival(video_dev, &frmival);
	if (ret != 0) {
		LOG_WRN("Could not set the framerate of %s", video_dev->name);
	}

	LOG_INF("Preparing %u buffers of %u bytes", CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, fmt.size);

	for (int i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
		vbuf = video_buffer_aligned_alloc(fmt.size, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
						  K_NO_WAIT);
		if (vbuf == NULL) {
			LOG_ERR("Could not allocate the video buffer");
			return -ENOMEM;
		}

		vbuf->type = VIDEO_BUF_TYPE_OUTPUT;

		ret = video_enqueue(video_dev, vbuf);
		if (ret != 0) {
			LOG_ERR("Could not enqueue video buffer");
			return ret;
		}
	}

	LOG_DBG("Preparing signaling for %s input/output", video_dev->name);

	k_poll_signal_init(&sig);
	k_poll_event_init(&evt[0], K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig);

	ret = video_set_signal(video_dev, &sig);
	if (ret != 0) {
		LOG_WRN("Failed to setup the signal on %s output endpoint", video_dev->name);
		timeout = K_MSEC(1);
	}

	ret = video_set_signal(uvc_dev, &sig);
	if (ret != 0) {
		LOG_ERR("Failed to setup the signal on %s input endpoint", uvc_dev->name);
		return ret;
	}

	LOG_INF("Starting the video transfer");

	ret = video_stream_start(video_dev, VIDEO_BUF_TYPE_OUTPUT);
	if (ret != 0) {
		LOG_ERR("Failed to start %s", video_dev->name);
		return ret;
	}

	while (true) {
		ret = k_poll(evt, ARRAY_SIZE(evt), timeout);
		if (ret != 0 && ret != -EAGAIN) {
			LOG_ERR("Poll exited with status %d", ret);
			return ret;
		}

		vbuf = &(struct video_buffer){.type = VIDEO_BUF_TYPE_OUTPUT};

		if (video_dequeue(video_dev, &vbuf, K_NO_WAIT) == 0) {
			LOG_DBG("Dequeued %p from %s, enqueueing to %s",
				(void *)vbuf, video_dev->name, uvc_dev->name);

			vbuf->type = VIDEO_BUF_TYPE_INPUT;

			ret = video_enqueue(uvc_dev, vbuf);
			if (ret != 0) {
				LOG_ERR("Could not enqueue video buffer to %s", uvc_dev->name);
				return ret;
			}
		}

		vbuf = &(struct video_buffer){.type = VIDEO_BUF_TYPE_INPUT};

		if (video_dequeue(uvc_dev, &vbuf, K_NO_WAIT) == 0) {
			LOG_DBG("Dequeued %p from %s, enqueueing to %s",
				(void *)vbuf, uvc_dev->name, video_dev->name);

			vbuf->type = VIDEO_BUF_TYPE_OUTPUT;

			ret = video_enqueue(video_dev, vbuf);
			if (ret != 0) {
				LOG_ERR("Could not enqueue video buffer to %s", video_dev->name);
				return ret;
			}
		}

		k_poll_signal_reset(&sig);
	}

	return 0;
}

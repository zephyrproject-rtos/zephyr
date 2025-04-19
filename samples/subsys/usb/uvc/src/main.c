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

const struct device *const uvc_dev = DEVICE_DT_GET(DT_NODELABEL(uvc));
const struct device *const video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));

int main(void)
{
	struct usbd_context *sample_usbd;
	struct video_buffer *vbuf;
	struct video_format fmt = {0};
	struct k_poll_signal sig;
	struct k_poll_event evt[1];
	k_timeout_t timeout = K_FOREVER;
	int ret;

	if (!device_is_ready(video_dev)) {
		LOG_ERR("video source %s failed to initialize", video_dev->name);
		return -ENODEV;
	}

	/* Must be done before initializing USB */
	uvc_set_video_dev(uvc_dev, video_dev);

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("Waiting the host to select the video format");

	/* Get the video format once it is selected by the host */
	while (true) {
		ret = video_get_format(uvc_dev, VIDEO_EP_IN, &fmt);
		if (ret == 0) {
			break;
		}
		if (ret != -EAGAIN) {
			LOG_ERR("Failed to get the video format");
			return ret;
		}

		k_sleep(K_MSEC(10));
	}

	LOG_INF("The host selected format %ux%u '%c%c%c%c'",
		fmt.width, fmt.height, fmt.pixelformat >> 24, fmt.pixelformat >> 16,
		fmt.pixelformat >> 8, fmt.pixelformat >> 0);

	LOG_INF("Preparing %u buffers of %u bytes",
		CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, fmt.pitch * fmt.height);

	for (int i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
		vbuf = video_buffer_alloc(fmt.pitch * fmt.height, K_NO_WAIT);
		if (vbuf == NULL) {
			LOG_ERR("Could not allocate the video buffer");
			return -ENOMEM;
		}

		ret = video_enqueue(video_dev, VIDEO_EP_OUT, vbuf);
		if (ret != 0) {
			LOG_ERR("Could not enqueue video buffer");
			return ret;
		}
	}

	LOG_DBG("Preparing signaling for %s input/output", video_dev->name);

	k_poll_signal_init(&sig);
	k_poll_event_init(&evt[0], K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig);

	ret = video_set_signal(video_dev, VIDEO_EP_OUT, &sig);
	if (ret != 0) {
		LOG_WRN("Failed to setup the signal on %s output endpoint", video_dev->name);
		timeout = K_MSEC(1);
	}

	ret = video_set_signal(uvc_dev, VIDEO_EP_IN, &sig);
	if (ret != 0) {
		LOG_ERR("Failed to setup the signal on %s input endpoint", uvc_dev->name);
		return ret;
	}

	LOG_INF("Starting the video transfer");

	ret = video_stream_start(video_dev);
	if (ret != 0) {
		LOG_ERR("Failed to start %s", video_dev->name);
	}

	while (true) {
		ret = k_poll(evt, ARRAY_SIZE(evt), timeout);
		if (ret != 0 && ret != -EAGAIN) {
			LOG_ERR("Poll exited with status %d", ret);
			return ret;
		}

		if (video_dequeue(video_dev, VIDEO_EP_OUT, &vbuf, K_NO_WAIT) == 0) {
			LOG_DBG("Dequeued %p from %s, enqueueing to %s",
				(void *)vbuf, video_dev->name, uvc_dev->name);

			ret = video_enqueue(uvc_dev, VIDEO_EP_IN, vbuf);
			if (ret != 0) {
				LOG_ERR("Could not enqueue video buffer to %s", uvc_dev->name);
				return ret;
			}
		}

		if (video_dequeue(uvc_dev, VIDEO_EP_IN, &vbuf, K_NO_WAIT) == 0) {
			LOG_DBG("Dequeued %p from %s, enqueueing to %s",
				(void *)vbuf, uvc_dev->name, video_dev->name);

			ret = video_enqueue(video_dev, VIDEO_EP_OUT, vbuf);
			if (ret != 0) {
				LOG_ERR("Could not enqueue video buffer to %s", video_dev->name);
				return ret;
			}
		}

		k_poll_signal_reset(&sig);
	}

	CODE_UNREACHABLE;
	return 0;
}

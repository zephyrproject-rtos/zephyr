/*
 * Copyright (c) 2024 tinyVision.ai Inc.
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

LOG_MODULE_REGISTER(uvc_sample, LOG_LEVEL_DBG);

int main(void)
{
	const struct device *uvc_dev = DEVICE_DT_GET(DT_NODELABEL(uvc));
	const struct device *rx_dev = DEVICE_DT_GET(DT_NODELABEL(mipi0));
	struct usbd_context *sample_usbd;
	struct video_buffer *vbuf;
	struct video_format fmt = {0};
	int ret;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret < 0) {
		return ret;
	}

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

	ret = video_set_format(rx_dev, VIDEO_EP_OUT, &fmt);
	if (ret < 0) {
		LOG_ERR("Failed to set %s format", rx_dev->name);
		return ret;
	}

	LOG_INF("Preparing %u buffers of %u bytes, %ux%u frame",
		CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, fmt.pitch * fmt.height, fmt.pitch, fmt.height);

	/* If this gets blocked, increase CONFIG_VIDEO_BUFFER_POOL_SZ_MAX */
	vbuf = video_buffer_alloc(fmt.pitch * fmt.height);
	if (vbuf == NULL) {
		LOG_ERR("Could not allocate the video buffer");
		return -ENOMEM;
	}

	ret = video_stream_start(rx_dev);
	if (ret < 0) {
		LOG_ERR("Failed to start %s", rx_dev->name);
		return ret;
	}

	LOG_INF("Starting video transfer");
	while (true) {
		LOG_DBG("enqueuing to %s", rx_dev->name);
		ret = video_enqueue(rx_dev, VIDEO_EP_OUT, vbuf);
		if (ret < 0) {
			LOG_ERR("Could not enqueue video buffer");
			return ret;
		}

		LOG_DBG("dequeueing from %s", rx_dev->name);
		ret = video_dequeue(rx_dev, VIDEO_EP_OUT, &vbuf, K_FOREVER);
		if (ret < 0) {
			LOG_ERR("Could not dequeue video buffer");
			return ret;
		}

		LOG_DBG("enqueuing to %s", uvc_dev->name);
		ret = video_enqueue(uvc_dev, VIDEO_EP_IN, vbuf);
		if (ret < 0) {
			LOG_ERR("Could not dequeue video buffer");
			return ret;
		}

		LOG_DBG("dequeueing from %s", uvc_dev->name);
		ret = video_dequeue(uvc_dev, VIDEO_EP_IN, &vbuf, K_FOREVER);
		if (ret < 0) {
			LOG_ERR("Could not enqueue video buffer");
			return ret;
		}
		LOG_DBG("dequeued vbuf %p", vbuf);

		LOG_INF("Transferred one more %ux%u frame", fmt.width, fmt.height);
	}

	return 0;
}

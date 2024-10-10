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
	const struct device *uvc_dev = DEVICE_DT_GET(DT_NODELABEL(uvc_0));
	struct usbd_context *sample_usbd;
	struct video_buffer *vbuf;
	struct video_format fmt = {0};
	int err;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	err = usbd_enable(sample_usbd);
	if (err) {
		return err;
	}

	/* Get the video format once it is selected by the host */
	while (true) {
		err = video_get_format(uvc_dev, VIDEO_EP_IN, &fmt);
		if (err == 0) {
			break;
		}
		if (err != -EAGAIN) {
			LOG_ERR("Failed to get the video format");
			return err;
		}

		k_sleep(K_MSEC(10));
	}

	LOG_INF("Preparing %u buffers of %u bytes, %ux%u frame",
		CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, fmt.pitch * fmt.height,
		fmt.pitch, fmt.height);

	/* Enqueue initial video frames */
	for (size_t i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
		/* If this gets blocked, increase CONFIG_USBD_VIDEO_BUFFER_POOL_SZ_MAX */
		vbuf = video_buffer_header_alloc(CONFIG_USBD_VIDEO_HEADER_SIZE,
						 fmt.pitch * fmt.height);
		if (vbuf == NULL) {
			LOG_ERR("Could not allocate the video buffer");
			return -ENOMEM;
		}

		memset(vbuf->buffer, 0x00, vbuf->size);
		vbuf->bytesused = vbuf->size;
		vbuf->flags = VIDEO_BUF_EOF;

		k_sleep(K_MSEC(10));

		LOG_INF("Transferring initial frame with size %ux%u, flags 0x%02x",
			fmt.width, fmt.height, vbuf->flags);

		err = video_enqueue(uvc_dev, VIDEO_EP_IN, vbuf);
		if (err != 0) {
			LOG_ERR("Could not enqueue video buffer");
			return err;
		}
	}

	LOG_DBG("Initial buffers submitted, resubmitting the completed buffers");

	/* As soon as a buffer is completed, submit it again */
	while (true) {
		err = video_dequeue(uvc_dev, VIDEO_EP_IN, &vbuf, K_FOREVER);
		if (err != 0) {
			LOG_ERR("Could not dequeue video buffer");
			return err;
		}

		k_sleep(K_MSEC(10));

		/* Toggle the color slightly at every frame */
		memset(vbuf->buffer, vbuf->buffer[0] + 0x0f, vbuf->size);
		vbuf->bytesused = vbuf->size;
		vbuf->flags = VIDEO_BUF_EOF;

		LOG_INF("Transferred one more frame at %ux%u, flags 0x%02x, submitting again",
		       fmt.width, fmt.height, vbuf->flags);

		err = video_enqueue(uvc_dev, VIDEO_EP_IN, vbuf);
		if (err != 0) {
			LOG_ERR("Could not enqueue video buffer");
			return err;
		}

		LOG_DBG("New buffer submitted, ");
	}

	return 0;
}

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
	int ret;

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	ret = usbd_enable(sample_usbd);
	if (ret) {
		return ret;
	}

	ret = video_get_format(uvc_dev, VIDEO_EP_IN, &fmt);
	if (ret != 0) {
		printk("could not read the format from UVC");
		return ret;
	}

	printk("Preparing %u buffers of %u bytes, %ux%u frame",
	       CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, fmt.pitch, fmt.height,
	       fmt.pitch * fmt.height);

	/* Initialize as many buffers as available */
	for (size_t i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
		vbuf = video_buffer_alloc(fmt.pitch * fmt.height);
		if (vbuf == NULL) {
			printk("could not allocate the video buffer");
			return -ENOMEM;
		}

		vbuf->bytesused = vbuf->size;
		vbuf->flags = VIDEO_BUF_EOF;

		ret = video_enqueue(uvc_dev, VIDEO_EP_IN, vbuf);
		if (ret != 0) {
			printk("could not enqueue video buffer");
			return ret;
		}
	}

	/* As soon as a buffer is completed, submit it again */
	while (true) {
		ret = video_dequeue(uvc_dev, VIDEO_EP_IN, &vbuf, K_FOREVER);
		if (ret != 0) {
			printk("could not dequeue video buffer");
			return ret;
		}

		printk("Transferred one more frame at %ux%u",
		       fmt.width, fmt.height);

		/* Toggle the color slightly at every frame */
		memset(vbuf->buffer, vbuf->buffer[0] ^ 0x0f, vbuf->size);

		ret = video_enqueue(uvc_dev, VIDEO_EP_IN, vbuf);
		if (ret != 0) {
			printk("could not enqueue video buffer");
			return ret;
		}
	}

	return 0;
}

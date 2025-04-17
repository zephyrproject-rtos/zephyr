/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>

#include <sample_usbd.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video/sw_pipeline.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/class/usbd_uvc.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(sw_pipeline_example, LOG_LEVEL_INF);

int main(void)
{
	const struct device *camera_dev;
	const struct device *pipe_dev;
	const struct device *uvc_dev;
	struct usbd_context *sample_usbd;
	struct video_buffer *vbuf;
	struct video_format fmt = {0};
	struct k_poll_signal sig = {0};
	struct k_poll_event evt[1] = {0};
	k_timeout_t timeout = K_FOREVER;
	int ret;

	camera_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));
	if (camera_dev == NULL || !device_is_ready(camera_dev)) {
		LOG_ERR("%s is not ready or failed to initialize", camera_dev->name);
		return -ENODEV;
	}

	pipe_dev = DEVICE_DT_GET(DT_NODELABEL(video_sw_pipeline));
	if (pipe_dev == NULL || !device_is_ready(pipe_dev)) {
		LOG_ERR("%s is not ready or failed to initialize", pipe_dev->name);
		return -ENODEV;
	}

	uvc_dev = DEVICE_DT_GET(DT_NODELABEL(uvc));
	if (uvc_dev == NULL || !device_is_ready(uvc_dev)) {
		LOG_ERR("%s is not ready or failed to initialize", uvc_dev->name);
		return -ENODEV;
	}

	video_sw_pipeline_set_source(pipe_dev, camera_dev);
	uvc_set_video_dev(uvc_dev, pipe_dev);

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

	LOG_INF("The host selected format %ux%u '%s'",
		fmt.width, fmt.height, VIDEO_FOURCC_TO_STR(fmt.pixelformat));

	LOG_INF("Preparing %u buffers of %u bytes",
		CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, fmt.pitch * fmt.height);

	/* Half of the buffers between the camera device and the pipeline device */
	for (int i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX / 2; i++) {
		vbuf = video_buffer_alloc(fmt.pitch * fmt.height, K_NO_WAIT);
		if (vbuf == NULL) {
			LOG_ERR("Failed to allocate the video buffer");
			return -ENOMEM;
		}

		ret = video_enqueue(camera_dev, VIDEO_EP_OUT, vbuf);
		if (ret != 0) {
			LOG_ERR("Failed to enqueue video buffer");
			return ret;
		}
	}

	/* Half of the buffers between the pipeline device and the sink device */
	for (int i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX / 2; i++) {
		vbuf = video_buffer_alloc(fmt.pitch * fmt.height, K_NO_WAIT);
		if (vbuf == NULL) {
			LOG_ERR("Failed to allocate the video buffer");
			return -ENOMEM;
		}

		ret = video_enqueue(pipe_dev, VIDEO_EP_OUT, vbuf);
		if (ret != 0) {
			LOG_ERR("Failed to enqueue video buffer");
			return ret;
		}
	}

	LOG_DBG("Preparing signaling for %s input/output", camera_dev->name);

	k_poll_signal_init(&sig);
	k_poll_event_init(&evt[0], K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig);

	ret = video_set_signal(camera_dev, VIDEO_EP_OUT, &sig);
	if (ret != 0) {
		LOG_WRN("Failed to setup the signal on %s output endpoint", camera_dev->name);
		timeout = K_MSEC(1);
	}

	ret = video_set_signal(pipe_dev, VIDEO_EP_OUT, &sig);
	if (ret != 0) {
		LOG_WRN("Failed to setup the signal on %s output endpoint", pipe_dev->name);
		timeout = K_MSEC(1);
	}

	ret = video_set_signal(uvc_dev, VIDEO_EP_IN, &sig);
	if (ret != 0) {
		LOG_WRN("Failed to setup the signal on %s input endpoint", uvc_dev->name);
		timeout = K_MSEC(1);
	}

	LOG_INF("Starting the video transfer");

	ret = video_stream_start(camera_dev);
	if (ret != 0) {
		LOG_ERR("Failed to start %s", camera_dev->name);
		return ret;
	}

	ret = video_stream_start(pipe_dev);
	if (ret != 0) {
		LOG_ERR("Failed to start %s", pipe_dev->name);
		return ret;
	}

	while (true) {
		ret = k_poll(evt, ARRAY_SIZE(evt), timeout);
		if (ret != 0 && ret != -EAGAIN) {
			LOG_ERR("Poll exited with status %d", ret);
			return ret;
		}

		if (video_dequeue(camera_dev, VIDEO_EP_OUT, &vbuf, K_NO_WAIT) == 0) {
			LOG_DBG("Dequeued %p from %s, enqueueing to %s",
				(void *)vbuf, camera_dev->name, uvc_dev->name);

			ret = video_enqueue(pipe_dev, VIDEO_EP_IN, vbuf);
			if (ret != 0) {
				LOG_ERR("Failed to enqueue video buffer to %s", pipe_dev->name);
				return ret;
			}
		}

		if (video_dequeue(pipe_dev, VIDEO_EP_IN, &vbuf, K_NO_WAIT) == 0) {
			LOG_DBG("Dequeued %p from %s, enqueueing to %s",
				(void *)vbuf, pipe_dev->name, uvc_dev->name);

			ret = video_enqueue(camera_dev, VIDEO_EP_OUT, vbuf);
			if (ret != 0) {
				LOG_ERR("Failed to enqueue video buffer to %s", uvc_dev->name);
				return ret;
			}
		}

		if (video_dequeue(pipe_dev, VIDEO_EP_OUT, &vbuf, K_NO_WAIT) == 0) {
			LOG_DBG("Dequeued %p from %s, enqueueing to %s",
				(void *)vbuf, pipe_dev->name, uvc_dev->name);

			ret = video_enqueue(uvc_dev, VIDEO_EP_IN, vbuf);
			if (ret != 0) {
				LOG_ERR("Failed to enqueue video buffer to %s", uvc_dev->name);
				return ret;
			}
		}

		if (video_dequeue(uvc_dev, VIDEO_EP_IN, &vbuf, K_NO_WAIT) == 0) {
			LOG_DBG("Dequeued %p from %s, enqueueing to %s",
				(void *)vbuf, uvc_dev->name, pipe_dev->name);

			ret = video_enqueue(pipe_dev, VIDEO_EP_OUT, vbuf);
			if (ret != 0) {
				LOG_ERR("Failed to enqueue video buffer to %s", pipe_dev->name);
				return ret;
			}
		}

		k_poll_signal_reset(&sig);
	}

	CODE_UNREACHABLE;
	return 0;
}

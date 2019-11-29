/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>

#include <drivers/video.h>
#include <drivers/display.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#define CAMERA_NODE DT_ALIAS(camera0)
#if DT_NODE_HAS_STATUS(CAMERA_NODE, okay)
#define VIDEO_CAMERA_DEV DT_LABEL(CAMERA_NODE)
#else
#warning No camera device specified, fallback to software pattern generator
#define VIDEO_CAMERA_DEV "VIDEO_SW_GENERATOR"
#endif

#define DISPLAY_NODE DT_ALIAS(display0)
#if DT_NODE_HAS_STATUS(DISPLAY_NODE, okay)
#define VIDEO_DISPLAY_DEV DT_LABEL(DISPLAY_NODE)
#else
#error No display device specified
#endif

void video_pipeline_loop(struct device *camera, struct device *display)
{
	struct k_poll_signal signal_camera, signal_display;
	unsigned int signaled;
	int result;

	k_poll_signal_init(&signal_camera);
	k_poll_signal_init(&signal_display);

	struct k_poll_event events[2] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &signal_camera),
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &signal_display),
	};

	video_set_signal(camera, VIDEO_EP_OUT, &signal_camera);
	video_set_signal(display, VIDEO_EP_IN, &signal_display);

	while (1) {
		k_poll(events, 2, K_FOREVER);

		/* CAMERA OUT -> DISPLAY IN */
		k_poll_signal_check(&signal_camera, &signaled, &result);
		if (signaled) {
			struct video_buffer *vbuf;

			k_poll_signal_reset(&signal_camera);

			while (!video_dequeue(camera, VIDEO_EP_OUT, &vbuf,
					      K_NO_WAIT)) {
				/* foward to display */
				video_enqueue(display, VIDEO_EP_IN, vbuf);
			}
		}

		/* Recycle DISPLAY IN -> CAMERA OUT */
		k_poll_signal_check(&signal_display, &signaled, &result);
		if (signaled) {
			struct video_buffer *vbuf;

			k_poll_signal_reset(&signal_display);

			while (!video_dequeue(display, VIDEO_EP_IN, &vbuf,
					      K_NO_WAIT)) {
				/* recycle for camera capture */
				video_enqueue(camera, VIDEO_EP_OUT, vbuf);
			}
		}

	}
}

void main(void)
{
	struct video_format fmt_display, fmt_camera;
	struct device *camera, *display;
	struct video_buffer *buffers[3];
	struct video_caps caps;
	size_t bsize;
	int i = 0;

	/* Retrieve camera device */
	camera = device_get_binding(VIDEO_CAMERA_DEV);
	if (camera == NULL) {
		LOG_ERR("Camera device %s not found.", VIDEO_CAMERA_DEV);
		return;
	}

	/* Retrieve display device */
	display = device_get_binding(VIDEO_DISPLAY_DEV);
	if (display == NULL) {
		LOG_ERR("Display device %s not found.", VIDEO_DISPLAY_DEV);
		return;
	}

	/* Get Display capabilities */
	if (video_get_caps(display, VIDEO_EP_IN, &caps)) {
		LOG_ERR("Unable to retrieve display capabilities");
		return;
	}

	printk("- Display (%s) Capabilities:\n",  VIDEO_DISPLAY_DEV);
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];

		printk("  %c%c%c%c width [%u; %u; %u] height [%u; %u; %u]\n",
		       (char)fcap->pixelformat,
		       (char)(fcap->pixelformat >> 8),
		       (char)(fcap->pixelformat >> 16),
		       (char)(fcap->pixelformat >> 24),
		       fcap->width_min, fcap->width_max, fcap->width_step,
		       fcap->height_min, fcap->height_max, fcap->height_step);
		i++;
	}

	if (video_get_format(display, VIDEO_EP_IN, &fmt_display)) {
		LOG_ERR("Unable to retrieve video format");
		return;
	}

	printk("  Default format: %c%c%c%c %ux%u\n",
	       (char)fmt_display.pixelformat,
	       (char)(fmt_display.pixelformat >> 8),
	       (char)(fmt_display.pixelformat >> 16),
	       (char)(fmt_display.pixelformat >> 24),
	       fmt_display.width, fmt_display.height);

	/* Get Camera capabilities */
	if (video_get_caps(camera, VIDEO_EP_OUT, &caps)) {
		LOG_ERR("Unable to retrieve camera capabilities");
		return;
	}

	printk("- Camera (%s) Capabilities:\n", VIDEO_CAMERA_DEV);
	i = 0;
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		/* fourcc to string */
		printk("  %c%c%c%c width [%u; %u; %u] height [%u; %u; %u]\n",
		       (char)fcap->pixelformat,
		       (char)(fcap->pixelformat >> 8),
		       (char)(fcap->pixelformat >> 16),
		       (char)(fcap->pixelformat >> 24),
		       fcap->width_min, fcap->width_max, fcap->width_step,
		       fcap->height_min, fcap->height_max, fcap->height_step);
		i++;
	}

	/* Get default/native format */
	if (video_get_format(camera, VIDEO_EP_OUT, &fmt_camera)) {
		LOG_ERR("Unable to retrieve camera format");
		return;
	}

	printk("  Default format: %c%c%c%c %ux%u\n",
	       (char)fmt_camera.pixelformat,
	       (char)(fmt_camera.pixelformat >> 8),
	       (char)(fmt_camera.pixelformat >> 16),
	       (char)(fmt_camera.pixelformat >> 24),
	       fmt_camera.width, fmt_camera.height);

	/* We should automatically find a common supported format, but for now
	 * try to use align with default formats.
	 */
	if (video_set_format(display, VIDEO_EP_IN, &fmt_camera)) {
		if (video_set_format(camera, VIDEO_EP_OUT, &fmt_display)) {
			LOG_ERR("Unable to align camera/display formats");
			return;
		}
		bsize = fmt_display.pitch * fmt_display.height;
	} else {
		bsize = fmt_camera.pitch * fmt_camera.height;
	}

	/* Alloc video buffers and enqueue for capture */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		buffers[i] = video_buffer_alloc(bsize);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return;
		}

		video_enqueue(camera, VIDEO_EP_OUT, buffers[i]);
	}

	/* Start camera */
	if (video_stream_start(camera)) {
		LOG_ERR("Unable to start capture (interface)");
		return;
	}

	/* Start display */
	if (video_stream_start(display)) {
		LOG_ERR("Unable to start display (interface)");
		return;
	}

	printk("Capture started\n");

	video_pipeline_loop(camera, display);
}

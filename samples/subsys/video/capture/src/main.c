/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define VIDEO_DEV_SW "VIDEO_SW_GENERATOR"

int main(void)
{
	struct video_buffer *buffers[2], *vbuf;
	struct video_format fmt;
	struct video_caps caps;
	const struct device *video_dev;
	unsigned int frame = 0;
	size_t bsize;
	int i = 0;

#if DT_HAS_CHOSEN(zephyr_camera)
	video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));
	if (!device_is_ready(video_dev)) {
		LOG_ERR("%s device is not ready", video_dev->name);
		return 0;
	}
#else
	video_dev = device_get_binding(VIDEO_DEV_SW);
	if (video_dev == NULL) {
		LOG_ERR("%s device not found", VIDEO_DEV_SW);
		return 0;
	}
#endif

	printk("Video device: %s\n", video_dev->name);

	/* Get capabilities */
	if (video_get_caps(video_dev, VIDEO_EP_OUT, &caps)) {
		LOG_ERR("Unable to retrieve video capabilities");
		return 0;
	}

	printk("- Capabilities:\n");
	while (caps.format_caps[i].pixelformat) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		/* fourcc to string */
		printk("  %c%c%c%c width [%u; %u; %u] height [%u; %u; %u]\n",
		       (char)fcap->pixelformat, (char)(fcap->pixelformat >> 8),
		       (char)(fcap->pixelformat >> 16), (char)(fcap->pixelformat >> 24),
		       fcap->width_min, fcap->width_max, fcap->width_step, fcap->height_min,
		       fcap->height_max, fcap->height_step);
		i++;
	}

	/* Get default/native format */
	if (video_get_format(video_dev, VIDEO_EP_OUT, &fmt)) {
		LOG_ERR("Unable to retrieve video format");
		return 0;
	}

	printk("- Default format: %c%c%c%c %ux%u\n", (char)fmt.pixelformat,
	       (char)(fmt.pixelformat >> 8), (char)(fmt.pixelformat >> 16),
	       (char)(fmt.pixelformat >> 24), fmt.width, fmt.height);

#if DT_HAS_CHOSEN(zephyr_display)
	struct display_capabilities capabilities;
	struct display_buffer_descriptor buf_desc;
	const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found", display_dev->name);
		return 0;
	}

	printk("\nDisplay device: %s\n", display_dev->name);

	display_get_capabilities(display_dev, &capabilities);

	printk("- Capabilities:\n");
	printk("  x_resolution = %u, y_resolution = %u, supported_pixel_formats = %u\n"
	       "  current_pixel_format = %u, current_orientation = %u\n\n",
	       capabilities.x_resolution, capabilities.y_resolution,
	       capabilities.supported_pixel_formats, capabilities.current_pixel_format,
	       capabilities.current_orientation);

#if defined(CONFIG_VIDEO_MCUX_MIPI_CSI2RX)
	/* Default output pixel format of the camera pipeline on i.MX RT11XX is XRGB32 */
	display_set_pixel_format(display_dev, PIXEL_FORMAT_ARGB_8888);
#endif

#endif

	/* Size to allocate for each buffer */
	bsize = fmt.pitch * fmt.height;

	/* Alloc video buffers and enqueue for capture */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		buffers[i] = video_buffer_alloc(bsize);
		if (buffers[i] == NULL) {
			LOG_ERR("Unable to alloc video buffer");
			return 0;
		}

		video_enqueue(video_dev, VIDEO_EP_OUT, buffers[i]);
	}

	/* Start video capture */
	if (video_stream_start(video_dev)) {
		LOG_ERR("Unable to start capture (interface)");
		return 0;
	}

	printk("Capture started\n");

	/* Grab video frames */
	while (1) {
		int err;

		err = video_dequeue(video_dev, VIDEO_EP_OUT, &vbuf, K_FOREVER);
		if (err) {
			LOG_ERR("Unable to dequeue video buf");
			return 0;
		}

		printk("\rGot frame %u! size: %u; timestamp %u ms\n", frame++, vbuf->bytesused,
		       vbuf->timestamp);

#if DT_HAS_CHOSEN(zephyr_display)
		buf_desc.buf_size = vbuf->bytesused;
		buf_desc.width = fmt.width;
		buf_desc.pitch = buf_desc.width;
		buf_desc.height = fmt.height;

		display_write(display_dev, 0, 0, &buf_desc, vbuf->buffer);

		display_blanking_off(display_dev);
#endif
		err = video_enqueue(video_dev, VIDEO_EP_OUT, vbuf);
		if (err) {
			LOG_ERR("Unable to requeue video buf");
			return 0;
		}
	}
}

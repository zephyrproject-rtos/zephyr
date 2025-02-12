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

#if DT_HAS_CHOSEN(zephyr_display)
static inline int display_setup(const struct device *const display_dev, const uint32_t pixfmt)
{
	struct display_capabilities capabilities;
	int ret = 0;

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device %s not found", display_dev->name);
		return -ENODEV;
	}

	printk("\nDisplay device: %s\n", display_dev->name);

	display_get_capabilities(display_dev, &capabilities);

	printk("- Capabilities:\n");
	printk("  x_resolution = %u, y_resolution = %u, supported_pixel_formats = %u\n"
	       "  current_pixel_format = %u, current_orientation = %u\n\n",
	       capabilities.x_resolution, capabilities.y_resolution,
	       capabilities.supported_pixel_formats, capabilities.current_pixel_format,
	       capabilities.current_orientation);

	/* Set display pixel format to match the one in use by the camera */
	switch (pixfmt) {
	case VIDEO_PIX_FMT_RGB565:
		ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_BGR_565);
		break;
	case VIDEO_PIX_FMT_XRGB32:
		ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_ARGB_8888);
		break;
	default:
		return -ENOTSUP;
	}

	if (ret) {
		LOG_ERR("Unable to set display format");
		return ret;
	}

	return display_blanking_off(display_dev);
}

static inline void video_display_frame(const struct device *const display_dev,
				       const struct video_buffer *const vbuf,
				       const struct video_format fmt)
{
	struct display_buffer_descriptor buf_desc;

	buf_desc.buf_size = vbuf->bytesused;
	buf_desc.width = fmt.width;
	buf_desc.pitch = buf_desc.width;
	buf_desc.height = fmt.height;

	display_write(display_dev, 0, 0, &buf_desc, vbuf->buffer);
}
#endif

int main(void)
{
	struct video_buffer *buffers[2], *vbuf;
	struct video_format fmt;
	struct video_caps caps;
	unsigned int frame = 0;
	size_t bsize;
	int i = 0;
	int err;

#if DT_HAS_CHOSEN(zephyr_camera)
	const struct device *const video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));

	if (!device_is_ready(video_dev)) {
		LOG_ERR("%s: video device is not ready", video_dev->name);
		return 0;
	}
#else
	const struct device *const video_dev = device_get_binding(VIDEO_DEV_SW);

	if (video_dev == NULL) {
		LOG_ERR("%s: video device not found or failed to initialized", VIDEO_DEV_SW);
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
	bsize = fmt.pitch * fmt.height;

	/* Alloc video buffers and enqueue for capture */
	for (i = 0; i < ARRAY_SIZE(buffers); i++) {
		/*
		 * For some hardwares, such as the PxP used on i.MX RT1170 to do image rotation,
		 * buffer alignment is needed in order to achieve the best performance
		 */
		buffers[i] = video_buffer_aligned_alloc(bsize, CONFIG_VIDEO_BUFFER_POOL_ALIGN);
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
		err = video_dequeue(video_dev, VIDEO_EP_OUT, &vbuf, K_FOREVER);
		if (err) {
			LOG_ERR("Unable to dequeue video buf");
			return 0;
		}

		printk("Got frame %u! size: %u; timestamp %u ms\n", frame++, vbuf->bytesused,
		       vbuf->timestamp);

#if DT_HAS_CHOSEN(zephyr_display)
		video_display_frame(display_dev, vbuf, fmt);
#endif

		err = video_enqueue(video_dev, VIDEO_EP_OUT, vbuf);
		if (err) {
			LOG_ERR("Unable to requeue video buf");
			return 0;
		}
	}
}

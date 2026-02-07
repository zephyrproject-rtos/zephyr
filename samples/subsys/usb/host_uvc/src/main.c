/*
 * Copyright 2025 - 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/device.h>

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));
static uint8_t convert_buffer[CONFIG_VIDEO_FRAME_WIDTH * CONFIG_VIDEO_FRAME_HEIGHT * 2];

#if DT_HAS_CHOSEN(zephyr_display)
static const char *pixel_format_name(enum display_pixel_format format)
{
	switch (format) {
	case PIXEL_FORMAT_RGB_565:
		return "RGB565";
	case PIXEL_FORMAT_RGB_565X:
		return "RGB565X";
	case PIXEL_FORMAT_RGB_888:
		return "RGB888";
	case PIXEL_FORMAT_ARGB_8888:
		return "ARGB8888";
	case PIXEL_FORMAT_MONO01:
		return "MONO01";
	case PIXEL_FORMAT_MONO10:
		return "MONO10";
	default:
		return "Unknown";
	}
}

static inline int display_setup(const struct device *const display_dev, const uint32_t pixfmt)
{
	struct display_capabilities capabilities;
	int ret = 0;

	LOG_INF("Display device: %s", display_dev->name);

	display_get_capabilities(display_dev, &capabilities);

	LOG_INF("Display Capabilities:");
	LOG_INF("  Resolution: %u * %u pixels", capabilities.x_resolution,
		capabilities.y_resolution);
	LOG_INF("  Current format: %s (0x%02x)",
		pixel_format_name(capabilities.current_pixel_format),
		capabilities.current_pixel_format);

	ret = display_blanking_off(display_dev);
	if (ret == -ENOSYS) {
		LOG_DBG("Display blanking off not available");
		ret = 0;
	}

	return ret;
}

/* TODO: Need to consider about using libMP */
static int yuyv_to_bgr565_convert(const uint8_t *yuyv_data, size_t yuyv_size, uint8_t **bgr565_data,
				  size_t *bgr565_size, uint16_t width, uint16_t height)
{
	size_t required_size = width * height * 2;
	uint16_t *bgr565_out = (uint16_t *)convert_buffer;
	const uint8_t *yuyv_in = yuyv_data;
	int output_idx = 0;

	for (int pixel_pair = 0; pixel_pair < (width * height) / 2; pixel_pair++) {
		int y_val0 = yuyv_in[0];
		int u_val = yuyv_in[1] - 128;
		int y_val1 = yuyv_in[2];
		int v_val = yuyv_in[3] - 128;

		int r0 = y_val0 + ((1436 * v_val) >> 10);
		int g0 = y_val0 - ((354 * u_val + 732 * v_val) >> 10);
		int b0 = y_val0 + ((1814 * u_val) >> 10);

		r0 = r0 < 0 ? 0 : (r0 > 255 ? 255 : r0);
		g0 = g0 < 0 ? 0 : (g0 > 255 ? 255 : g0);
		b0 = b0 < 0 ? 0 : (b0 > 255 ? 255 : b0);

		int r1 = y_val1 + ((1436 * v_val) >> 10);
		int g1 = y_val1 - ((354 * u_val + 732 * v_val) >> 10);
		int b1 = y_val1 + ((1814 * u_val) >> 10);

		r1 = r1 < 0 ? 0 : (r1 > 255 ? 255 : r1);
		g1 = g1 < 0 ? 0 : (g1 > 255 ? 255 : g1);
		b1 = b1 < 0 ? 0 : (b1 > 255 ? 255 : b1);

		if (output_idx < (width * height)) {
			bgr565_out[output_idx] = ((b0 >> 3) << 11) | ((g0 >> 2) << 5) | (r0 >> 3);
			output_idx++;
		}

		if (output_idx < (width * height)) {
			bgr565_out[output_idx] = ((b1 >> 3) << 11) | ((g1 >> 2) << 5) | (r1 >> 3);
			output_idx++;
		}

		yuyv_in += 4;
	}

	if (output_idx != width * height) {
		LOG_WRN("Output pixel count mismatch: expected=%d, got=%d", width * height,
			output_idx);
	}

	*bgr565_data = convert_buffer;
	*bgr565_size = required_size;

	LOG_DBG("Successfully converted YUYV to BGR565: %ux%u (%zu bytes)", width, height,
		required_size);

	return 0;
}

static inline void video_display_frame(const struct device *const display_dev,
				       const struct video_buffer *const vbuf,
				       const struct video_format fmt)
{
	if (!vbuf || !vbuf->buffer) {
		LOG_ERR("Invalid vbuf or buffer pointer");
		return;
	}

	LOG_DBG("Display frame: format=0x%x, size=%u, buffer=%p", fmt.pixelformat, vbuf->bytesused,
		vbuf->buffer);

	if (fmt.pixelformat == VIDEO_PIX_FMT_YUYV) {
		uint8_t *rgb565_data = NULL;
		size_t rgb565_size = 0;
		int ret;

		LOG_DBG("Converting YUYV to RGB565: %ux%u", fmt.width, fmt.height);

		ret = yuyv_to_bgr565_convert(vbuf->buffer, vbuf->bytesused, &rgb565_data,
					     &rgb565_size, fmt.width, fmt.height);
		if (ret != 0) {
			LOG_ERR("Failed to convert YUYV to RGB565: %d", ret);
			return;
		}

		struct display_buffer_descriptor buf_desc = {
			.buf_size = rgb565_size,
			.width = fmt.width,
			.pitch = fmt.width,
			.height = fmt.height,
		};

		int display_ret = display_write(display_dev, 0, 0, &buf_desc, rgb565_data);

		if (display_ret != 0) {
			LOG_ERR("Failed to write converted frame to display: %d", display_ret);
		}
	}
}
#endif

int main(void)
{
	const struct device *uvc_host = device_get_binding("usbh_uvc_0");
	struct video_buffer *vbuf = &(struct video_buffer){};
	struct video_buffer *allocated_vbufs[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX] = {NULL};
	struct video_format fmt;
	struct video_caps caps;
	struct video_frmival frmival;
	struct video_frmival_enum fie;
	struct k_poll_signal sig;
	struct k_poll_event evt[1];
	k_timeout_t timeout = K_FOREVER;
	enum video_buf_type type = VIDEO_BUF_TYPE_OUTPUT;
#if (CONFIG_VIDEO_SOURCE_CROP_WIDTH && CONFIG_VIDEO_SOURCE_CROP_HEIGHT) ||                         \
	CONFIG_VIDEO_FRAME_HEIGHT || CONFIG_VIDEO_FRAME_WIDTH
	struct video_selection sel = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};
#endif
	size_t bsize;
	uint8_t i = 0;
	int err;
	int allocated_count = 0;
	int signaled, result;
	int tp_set_ret = -ENOTSUP;

#if DT_HAS_CHOSEN(zephyr_display)
	const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	bool display_configured = false;
#endif

	if (IS_ENABLED(CONFIG_VIDEO_SHELL)) {
		LOG_INF("Letting the user control the device with the video shell");
		return 0;
	}

	if (!device_is_ready(uvc_host)) {
		LOG_ERR("%s: USB host is not ready", uvc_host->name);
		return 0;
	}
	LOG_INF("USB host: %s", uvc_host->name);

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

	k_poll_signal_init(&sig);
	k_poll_event_init(&evt[0], K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig);

	err = video_set_signal(uvc_host, &sig);
	if (err != 0) {
		LOG_WRN("Failed to setup the signal on %s output endpoint", uvc_host->name);
		timeout = K_MSEC(10);
	}

	while (true) {
		err = k_poll(evt, ARRAY_SIZE(evt), timeout);
		if (err != 0 && err != -EAGAIN) {
			LOG_WRN("Poll failed with error %d, retrying...", err);
			continue;
		}

		k_poll_signal_check(&sig, &signaled, &result);
		if (!signaled) {
			continue;
		}

		k_poll_signal_reset(&sig);

		switch (result) {
		case VIDEO_LINK_UP:
			LOG_INF("UVC device connected successfully!");

			caps.type = type;
			if (video_get_caps(uvc_host, &caps)) {
				LOG_ERR("Unable to retrieve video capabilities");
				break;
			}

			LOG_INF("- Capabilities:");
			i = 0;
			while (caps.format_caps[i].pixelformat) {
				LOG_INF("  %s width [%u; %u; %u] height [%u; %u; %u]",
					VIDEO_FOURCC_TO_STR(caps.format_caps[i].pixelformat),
					caps.format_caps[i].width_min,
					caps.format_caps[i].width_max,
					caps.format_caps[i].width_step,
					caps.format_caps[i].height_min,
					caps.format_caps[i].height_max,
					caps.format_caps[i].height_step);
				i++;
			}

			fmt.type = type;
			if (video_get_format(uvc_host, &fmt)) {
				LOG_ERR("Unable to retrieve video format");
				break;
			}

#if CONFIG_VIDEO_SOURCE_CROP_WIDTH && CONFIG_VIDEO_SOURCE_CROP_HEIGHT
			sel.target = VIDEO_SEL_TGT_CROP;
			sel.rect.left = CONFIG_VIDEO_SOURCE_CROP_LEFT;
			sel.rect.top = CONFIG_VIDEO_SOURCE_CROP_TOP;
			sel.rect.width = CONFIG_VIDEO_SOURCE_CROP_WIDTH;
			sel.rect.height = CONFIG_VIDEO_SOURCE_CROP_HEIGHT;
			if (video_set_selection(uvc_host, &sel)) {
				LOG_ERR("Unable to set selection crop");
				break;
			}
			LOG_INF("Selection crop set to (%u,%u)/%ux%u", sel.rect.left, sel.rect.top,
				sel.rect.width, sel.rect.height);
#endif

#if CONFIG_VIDEO_FRAME_HEIGHT || CONFIG_VIDEO_FRAME_WIDTH
#if CONFIG_VIDEO_FRAME_HEIGHT
			fmt.height = CONFIG_VIDEO_FRAME_HEIGHT;
#endif

#if CONFIG_VIDEO_FRAME_WIDTH
			fmt.width = CONFIG_VIDEO_FRAME_WIDTH;
#endif

			sel.target = VIDEO_SEL_TGT_CROP;
			err = video_get_selection(uvc_host, &sel);
			if (err < 0 && err != -ENOSYS) {
				LOG_ERR("Unable to get selection crop");
				break;
			}

			if (err == 0 &&
			    (sel.rect.width != fmt.width || sel.rect.height != fmt.height)) {
				sel.target = VIDEO_SEL_TGT_COMPOSE;
				sel.rect.left = 0;
				sel.rect.top = 0;
				sel.rect.width = fmt.width;
				sel.rect.height = fmt.height;
				err = video_set_selection(uvc_host, &sel);
				if (err < 0 && err != -ENOSYS) {
					LOG_ERR("Unable to set selection compose");
					break;
				}
			}
#endif

			if (strcmp(CONFIG_VIDEO_PIXEL_FORMAT, "")) {
				fmt.pixelformat = VIDEO_FOURCC_FROM_STR(CONFIG_VIDEO_PIXEL_FORMAT);
			}
#if CONFIG_VIDEO_FRAME_WIDTH > 0
			fmt.width = CONFIG_VIDEO_FRAME_WIDTH;
#endif

#if CONFIG_VIDEO_FRAME_HEIGHT > 0
			fmt.height = CONFIG_VIDEO_FRAME_HEIGHT;
#endif

			LOG_INF("- Expected video format: %s %ux%u",
				VIDEO_FOURCC_TO_STR(fmt.pixelformat), fmt.width, fmt.height);

			if (video_set_format(uvc_host, &fmt)) {
				LOG_ERR("Unable to set format");
				break;
			}

			if (!video_get_frmival(uvc_host, &frmival)) {
				LOG_INF("- Default frame rate : %f fps",
					1.0 * frmival.denominator / frmival.numerator);
			}

			LOG_INF("- Supported frame intervals for the default format:");
			memset(&fie, 0, sizeof(fie));
			fie.format = &fmt;
			while (video_enum_frmival(uvc_host, &fie) == 0) {
				if (fie.type == VIDEO_FRMIVAL_TYPE_DISCRETE) {
					LOG_INF("   %u/%u", fie.discrete.numerator,
						fie.discrete.denominator);
				} else {
					LOG_INF("   [min = %u/%u; max = %u/%u; step = %u/%u]",
						fie.stepwise.min.numerator,
						fie.stepwise.min.denominator,
						fie.stepwise.max.numerator,
						fie.stepwise.max.denominator,
						fie.stepwise.step.numerator,
						fie.stepwise.step.denominator);
				}
				fie.index++;
			}

#if CONFIG_VIDEO_TARGET_FPS > 0
			frmival.denominator = CONFIG_VIDEO_TARGET_FPS;
			frmival.numerator = 1;
			if (!video_set_frmival(uvc_host, &frmival)) {
				if (!video_get_frmival(uvc_host, &frmival)) {
					LOG_INF("- Target frame rate set to: %f fps",
						1.0 * frmival.denominator / frmival.numerator);
				}
			}
#endif

			LOG_INF("- Supported controls:");
			const struct device *last_dev = NULL;
			struct video_ctrl_query cq = {.dev = uvc_host,
						      .id = VIDEO_CTRL_FLAG_NEXT_CTRL};

			while (!video_query_ctrl(&cq)) {
				if (cq.dev != last_dev) {
					last_dev = cq.dev;
					LOG_INF("\t\tdevice: %s", cq.dev->name);
				}
				video_print_ctrl(&cq);
				cq.id |= VIDEO_CTRL_FLAG_NEXT_CTRL;
			}

			struct video_control ctrl = {.id = VIDEO_CID_HFLIP, .val = 1};

			if (IS_ENABLED(CONFIG_VIDEO_CTRL_HFLIP)) {
				video_set_ctrl(uvc_host, &ctrl);
			}

			if (IS_ENABLED(CONFIG_VIDEO_CTRL_VFLIP)) {
				ctrl.id = VIDEO_CID_VFLIP;
				video_set_ctrl(uvc_host, &ctrl);
			}

			if (IS_ENABLED(CONFIG_TEST)) {
				ctrl.id = VIDEO_CID_TEST_PATTERN;
				tp_set_ret = video_set_ctrl(uvc_host, &ctrl);
			}

#if DT_HAS_CHOSEN(zephyr_display)
			if (!display_configured && device_is_ready(display_dev)) {
				err = display_setup(display_dev, fmt.pixelformat);
				if (err) {
					LOG_ERR("Unable to set up display: %d", err);
				} else {
					display_configured = true;
					LOG_INF("Display configured successfully");
				}
			}
#endif

			bsize = fmt.width * fmt.height * 2;

			if (caps.min_vbuf_count > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX ||
			    bsize > (CONFIG_VIDEO_FRAME_WIDTH * CONFIG_VIDEO_FRAME_HEIGHT * 2)) {
				LOG_ERR("Not enough buffers or memory to start streaming");
				break;
			}

			LOG_INF("Allocating %d video buffers, size=%zu",
				CONFIG_VIDEO_BUFFER_POOL_NUM_MAX, bsize);

			for (i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
				vbuf = video_buffer_aligned_alloc(
					bsize, CONFIG_VIDEO_BUFFER_POOL_ALIGN, K_FOREVER);
				if (vbuf == NULL) {
					LOG_ERR("Unable to alloc video buffer %d/%d", i,
						CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
					break;
				}
				allocated_vbufs[i] = vbuf;
				allocated_count = i + 1;
				vbuf->type = type;
				video_enqueue(uvc_host, vbuf);
			}

			if (i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX) {
				LOG_ERR("Only allocated %d/%d buffers!", i,
					CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
			}

			if (video_stream_start(uvc_host, type)) {
				LOG_ERR("Unable to start capture (interface)");
				break;
			}

			LOG_INF("Capture started");
			break;

		case VIDEO_LINK_DOWN:
			video_stream_stop(uvc_host, type);
			for (i = 0; i < allocated_count; i++) {
				if (allocated_vbufs[i]) {
					video_buffer_release(allocated_vbufs[i]);
					allocated_vbufs[i] = NULL;
				}
			}
			allocated_count = 0;
			break;

		case VIDEO_BUF_DONE:
			err = video_dequeue(uvc_host, &vbuf, K_FOREVER);
			if (err != 0) {
				LOG_ERR("Unable to dequeue video buf");
				break;
			}

#ifdef CONFIG_TEST
			if (tp_set_ret < 0) {
				LOG_DBG("Test pattern control was not successful. Skip test");
			} else if (is_colorbar_ok(vbuf->buffer, fmt)) {
				LOG_DBG("Pattern OK!\n");
			}
#endif

#if DT_HAS_CHOSEN(zephyr_display)
			if (display_configured) {
				video_display_frame(display_dev, vbuf, fmt);
			}
#endif
			err = video_enqueue(uvc_host, vbuf);
			if (err == -ENODEV) {
				LOG_DBG("Device disconnected");
			} else if (err != 0) {
				LOG_ERR("Unable to requeue video buf: %d", err);
			}
			break;

		default:
			LOG_WRN("Received unknown signal: %d", result);
			break;
		}
	}
}

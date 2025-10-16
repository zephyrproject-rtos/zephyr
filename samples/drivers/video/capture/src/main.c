/*
 * Copyright (c) 2019 Linaro Limited
 * Copyright 2025 NXP
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

#ifdef CONFIG_TEST
#include "check_test_pattern.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);
#else
LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);
#endif

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));
const struct device *const uvc_host = DEVICE_DT_GET(DT_NODELABEL(uvc_host));
static uint8_t convert_buffer[CONFIG_VIDEO_BUFFER_POOL_SZ_MAX];

/* TODO: Need to consider about using pixfmt */
#if DT_HAS_CHOSEN(zephyr_display)
static inline int display_setup(const struct device *const display_dev, const uint32_t pixfmt)
{
	struct display_capabilities capabilities;
	int ret = 0;

	LOG_INF("Display device: %s", display_dev->name);

	display_get_capabilities(display_dev, &capabilities);

	LOG_INF("- Capabilities:");
	LOG_INF("  x_resolution = %u, y_resolution = %u, supported_pixel_formats = %u"
		"  current_pixel_format = %u, current_orientation = %u",
		capabilities.x_resolution, capabilities.y_resolution,
		capabilities.supported_pixel_formats, capabilities.current_pixel_format,
		capabilities.current_orientation);

	/* Set display pixel format to match the one in use by the camera */

	if (capabilities.current_pixel_format != PIXEL_FORMAT_BGR_565) {
		ret = display_set_pixel_format(display_dev, PIXEL_FORMAT_BGR_565);
	}

	if (ret) {
		LOG_ERR("Unable to set display format");
		return ret;
	}

	/* Turn off blanking if driver supports it */
	ret = display_blanking_off(display_dev);
	if (ret == -ENOSYS) {
		LOG_DBG("Display blanking off not available");
		ret = 0;
	}

	return ret;
}

static int yuyv_to_bgr565_convert(const uint8_t *yuyv_data, size_t yuyv_size,
                                  uint8_t **bgr565_data, size_t *bgr565_size,
                                  uint16_t width, uint16_t height)
{
    /* Calculate required buffer size for BGR565 */
    size_t required_size = width * height * 2;


    uint16_t *bgr565_out = (uint16_t *)convert_buffer;
    const uint8_t *yuyv_in = yuyv_data;

    /* Corrected loop: properly handle each pixel pair */
    int output_idx = 0;  // BGR565 output index

    for (int pixel_pair = 0; pixel_pair < (width * height) / 2; pixel_pair++) {
        /* Extract YUYV components */
        int y0 = yuyv_in[0];
        int u  = yuyv_in[1] - 128;
        int y1 = yuyv_in[2];
        int v  = yuyv_in[3] - 128;

        /* Convert first pixel (Y0UV) */
        int r0 = y0 + ((1436 * v) >> 10);
        int g0 = y0 - ((354 * u + 732 * v) >> 10);
        int b0 = y0 + ((1814 * u) >> 10);

        /* Clamp to 0-255 range */
        r0 = r0 < 0 ? 0 : (r0 > 255 ? 255 : r0);
        g0 = g0 < 0 ? 0 : (g0 > 255 ? 255 : g0);
        b0 = b0 < 0 ? 0 : (b0 > 255 ? 255 : b0);

        /* Convert second pixel (Y1UV) */
        int r1 = y1 + ((1436 * v) >> 10);
        int g1 = y1 - ((354 * u + 732 * v) >> 10);
        int b1 = y1 + ((1814 * u) >> 10);

        /* Clamp to 0-255 range */
        r1 = r1 < 0 ? 0 : (r1 > 255 ? 255 : r1);
        g1 = g1 < 0 ? 0 : (g1 > 255 ? 255 : g1);
        b1 = b1 < 0 ? 0 : (b1 > 255 ? 255 : b1);

        /* Safely store BGR565 pixels */
        if (output_idx < (width * height)) {
            bgr565_out[output_idx] = ((b0 >> 3) << 11) | ((g0 >> 2) << 5) | (r0 >> 3);
            output_idx++;
        }

        if (output_idx < (width * height)) {
            bgr565_out[output_idx] = ((b1 >> 3) << 11) | ((g1 >> 2) << 5) | (r1 >> 3);
            output_idx++;
        }

        yuyv_in += 4; /* Move to next YUYV data group */
    }

    /* Validate output index */
    if (output_idx != width * height) {
        LOG_WRN("Output pixel count mismatch: expected=%d, got=%d", width * height, output_idx);
    }

    *bgr565_data = convert_buffer;
    *bgr565_size = required_size;

    LOG_DBG("Successfully converted YUYV to BGR565: %ux%u (%zu bytes)",
            width, height, required_size);

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

    LOG_DBG("Display frame: format=0x%x, size=%u, buffer=%p",
            fmt.pixelformat, vbuf->bytesused, vbuf->buffer);

    /* Check if YUYV format requires conversion */
    if (fmt.pixelformat == VIDEO_PIX_FMT_YUYV) {
        uint8_t *rgb565_data = NULL;
        size_t rgb565_size = 0;
        int ret;

        LOG_DBG("Converting YUYV to RGB565: %ux%u", fmt.width, fmt.height);

        /* Perform color space conversion */
        ret = yuyv_to_bgr565_convert(vbuf->buffer, vbuf->bytesused,
                                     &rgb565_data, &rgb565_size,
                                     fmt.width, fmt.height);
        if (ret != 0) {
            LOG_ERR("Failed to convert YUYV to RGB565: %d", ret);
            return;
        }

        LOG_DBG("Conversion successful: %zu bytes", rgb565_size);

        /* Display using converted data */
        struct display_buffer_descriptor buf_desc = {
            .buf_size = rgb565_size,
            .width = fmt.width,
            .pitch = fmt.width,
            .height = fmt.height,
        };

        int display_ret = display_write(display_dev, 0, 0, &buf_desc, rgb565_data);
        if (display_ret != 0) {
            LOG_ERR("Failed to write converted frame to display: %d", display_ret);
        } else {
            LOG_DBG("Successfully displayed converted frame: %ux%u",
                    fmt.width, fmt.height);
        }

    } else {
        /* Original display logic for other formats */
        struct display_buffer_descriptor buf_desc = {
            .buf_size = vbuf->bytesused,
            .width = fmt.width,
            .pitch = fmt.width,
            .height = vbuf->bytesused / fmt.pitch,
        };

        int display_ret = display_write(display_dev, 0, vbuf->line_offset,
                                       &buf_desc, vbuf->buffer);
        if (display_ret != 0) {
            LOG_ERR("Failed to write frame to display: %d", display_ret);
        }
    }
}
#endif

static void video_uvc_device_signal_handler(void)
{
	/* TODO: This function is intentionally left empty.
	 * Implementation may be added in future if needed.
	 */
}

int main(void)
{
	struct video_buffer *vbuf = &(struct video_buffer){};
	struct video_format fmt;
	struct video_caps caps;
	struct video_frmival frmival;
	struct video_frmival_enum fie;
	struct k_poll_signal sig;
	struct k_poll_event evt[1];
	k_timeout_t timeout = K_FOREVER;
	enum video_buf_type type = VIDEO_BUF_TYPE_OUTPUT;
#if (CONFIG_VIDEO_SOURCE_CROP_WIDTH && CONFIG_VIDEO_SOURCE_CROP_HEIGHT) ||	\
	CONFIG_VIDEO_FRAME_HEIGHT || CONFIG_VIDEO_FRAME_WIDTH
	struct video_selection sel = {
		.type = VIDEO_BUF_TYPE_OUTPUT,
	};
#endif
	unsigned int frame = 0;
	size_t bsize;
	int i = 0;
	int err;
	int signaled, result;
	int tp_set_ret = -ENOTSUP;

#if DT_HAS_CHOSEN(zephyr_display)
	const struct device *const display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	bool display_configured = false;
#endif

	/* When the video shell is enabled, do not run the capture loop */
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
		case USBH_DEVICE_CONNECTED:
			LOG_INF("UVC device connected successfully!");

			/* Get capabilities */
			caps.type = type;
			if (video_get_caps(uvc_host, &caps)) {
				LOG_ERR("Unable to retrieve video capabilities");
				break;
			}

			LOG_INF("- Capabilities:");
			i = 0;
			while (caps.format_caps[i].pixelformat) {
				const struct video_format_cap *fcap = &caps.format_caps[i];
				/* fourcc to string */
				LOG_INF("  %s width [%u; %u; %u] height [%u; %u; %u]",
					VIDEO_FOURCC_TO_STR(fcap->pixelformat),
					fcap->width_min, fcap->width_max, fcap->width_step,
					fcap->height_min, fcap->height_max, fcap->height_step);
				i++;
			}

			/* Get default/native format */
			fmt.type = type;
			if (video_get_format(uvc_host, &fmt)) {
				LOG_ERR("Unable to retrieve video format");
				break;
			}

			/* Set the crop setting if necessary */
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
			LOG_INF("Selection crop set to (%u,%u)/%ux%u",
				sel.rect.left, sel.rect.top, sel.rect.width, sel.rect.height);
#endif

#if CONFIG_VIDEO_FRAME_HEIGHT || CONFIG_VIDEO_FRAME_WIDTH
#if CONFIG_VIDEO_FRAME_HEIGHT
			fmt.height = CONFIG_VIDEO_FRAME_HEIGHT;
#endif

#if CONFIG_VIDEO_FRAME_WIDTH
			fmt.width = CONFIG_VIDEO_FRAME_WIDTH;
#endif

			/*
				* Check (if possible) if targeted size is same as crop
				* and if compose is necessary
				*/
			sel.target = VIDEO_SEL_TGT_CROP;
			err = video_get_selection(uvc_host, &sel);
			if (err < 0 && err != -ENOSYS) {
				LOG_ERR("Unable to get selection crop");
				break;
			}

			if (err == 0 && (sel.rect.width != fmt.width || sel.rect.height != fmt.height)) {
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

			LOG_INF("- Video format: %s %ux%u",
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
					LOG_INF("   %u/%u", fie.discrete.numerator, fie.discrete.denominator);
				} else {
					LOG_INF("   [min = %u/%u; max = %u/%u; step = %u/%u]",
						fie.stepwise.min.numerator, fie.stepwise.min.denominator,
						fie.stepwise.max.numerator, fie.stepwise.max.denominator,
						fie.stepwise.step.numerator, fie.stepwise.step.denominator);
				}
				fie.index++;
			}

#if CONFIG_VIDEO_TARGET_FPS > 0
			frmival.denominator = CONFIG_VIDEO_TARGET_FPS;
			frmival.numerator = 1;
			if (!video_set_frmival(uvc_host, &frmival)) {
				/* Get the actual frame rate that was set */
				if (!video_get_frmival(uvc_host, &frmival)) {
					LOG_INF("- Target frame rate set to: %f fps",
						1.0 * frmival.denominator / frmival.numerator);
				}
			}
#endif

			/* Get supported controls */
			LOG_INF("- Supported controls:");
			const struct device *last_dev = NULL;
			struct video_ctrl_query cq = {.dev = uvc_host, .id = VIDEO_CTRL_FLAG_NEXT_CTRL};

			while (!video_query_ctrl(&cq)) {
				if (cq.dev != last_dev) {
					last_dev = cq.dev;
					LOG_INF("\t\tdevice: %s", cq.dev->name);
				}
				video_print_ctrl(&cq);
				cq.id |= VIDEO_CTRL_FLAG_NEXT_CTRL;
			}

			/* Set controls */
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

			/* Size to allocate for each buffer */
			if (caps.min_line_count == LINE_COUNT_HEIGHT) {
				bsize = fmt.width * fmt.height * 2;
			} else {
				bsize = fmt.pitch * caps.min_line_count;
			}

			/* Alloc video buffers and enqueue for capture */
			if (caps.min_vbuf_count > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX ||
				bsize > CONFIG_VIDEO_BUFFER_POOL_SZ_MAX) {
				LOG_ERR("Not enough buffers or memory to start streaming");
				break;
			}

			for (i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
				/*
					* For some hardwares, such as the PxP used on i.MX RT1170 to do image rotation,
					* buffer alignment is needed in order to achieve the best performance
					*/
				vbuf = video_buffer_aligned_alloc(bsize, CONFIG_VIDEO_BUFFER_POOL_ALIGN,
									K_FOREVER);
				if (vbuf == NULL) {
					LOG_ERR("Unable to alloc video buffer");
					break;
				}
				vbuf->type = type;
				video_enqueue(uvc_host, vbuf);
			}

			/* enable UVC device streaming */
			if (video_stream_start(uvc_host, type)) {
				LOG_ERR("Unable to start capture (interface)");
				break;
			}

			/* Delay to wait UVC device ready */
			k_msleep(500);

			LOG_INF("Capture started");
			break;

		case USBH_DEVICE_DISCONNECTED:
			/* TODO: CONFIG_VIDEO_BUFFER_POOL_NUM_MAX is 1 now, consider multiple video buffers */
			for (i = 0; i < CONFIG_VIDEO_BUFFER_POOL_NUM_MAX; i++) {
				err = video_dequeue(uvc_host, &vbuf, K_NO_WAIT);
				if (!err && vbuf) {
					video_buffer_release(vbuf);
				}
			}
			LOG_INF("UVC device disconnected!");
			break;

		case VIDEO_BUF_DONE:
			/* Process completed video buffer */
			err = video_dequeue(uvc_host, &vbuf, K_FOREVER);
			if (err) {
				LOG_ERR("Unable to dequeue video buf");
				break;
			}

			LOG_DBG("Got frame %u! size: %u; timestamp %u ms",
				frame++, vbuf->bytesused, vbuf->timestamp);

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
			if (err) {
				LOG_ERR("Unable to requeue video buf");
				break;
			}
			break;

		default:
			LOG_WRN("Received unknown signal: %d", result);
			break;
		}
	}
}

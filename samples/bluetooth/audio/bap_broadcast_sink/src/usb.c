/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink Sample LC3
 *
 * This files handles all the USB related functionality to audio out for the Sample
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/assigned_numbers.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/usb/usb_buf.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/clock.h>
#include <zephyr/toolchain.h>
#include <zephyr/usb/class/usbd_uac2.h>
#include <zephyr/usb/usbd.h>

#include <sample_usbd.h>

#include "lc3.h"
#include "stereo_out.h"
#include "usb.h"

LOG_MODULE_REGISTER(usb, CONFIG_LOG_DEFAULT_LEVEL);

#define USB_ENQUEUE_COUNT        30U /* 30 times 1ms frames => 30ms */
#define USB_FRAME_DURATION_US    1000U
#define USB_SAMPLE_CNT           ((USB_FRAME_DURATION_US * USB_SAMPLE_RATE_HZ) / USEC_PER_SEC)
#define USB_BYTES_PER_SAMPLE     sizeof(int16_t)
#define USB_MONO_FRAME_SIZE      (USB_SAMPLE_CNT * USB_BYTES_PER_SAMPLE)
#define USB_CHANNELS             2U
#define USB_STEREO_FRAME_SIZE    (USB_MONO_FRAME_SIZE * USB_CHANNELS)

#define IN_TERMINAL_ID UAC2_ENTITY_ID(DT_NODELABEL(in_terminal))

K_MEM_SLAB_DEFINE_STATIC(usb_in_buf_pool, ROUND_UP(USB_STEREO_FRAME_SIZE, UDC_BUF_GRANULARITY),
			 USB_ENQUEUE_COUNT, UDC_BUF_ALIGN);
static volatile bool terminal_enabled;

/* USB consumer callback, called every 1ms, consumes data from ring-buffer */
static void uac2_sof_cb(const struct device *dev, void *user_data)
{
	void *pcm_buf;
	uint32_t size;
	int err;

	ARG_UNUSED(user_data);

	if (!terminal_enabled) {
		/* Simply discard the data then */
		(void)stereo_out_read(NULL, USB_STEREO_FRAME_SIZE);
		return;
	}

	err = k_mem_slab_alloc(&usb_in_buf_pool, &pcm_buf, K_NO_WAIT);
	if (err != 0) {
		LOG_WRN("Could not allocate pcm_buf");
		return;
	}

	size = stereo_out_read(pcm_buf, USB_STEREO_FRAME_SIZE);
	if (size != USB_STEREO_FRAME_SIZE) {
		/* If we could not fill the buffer, zero-fill the rest (possibly all) */
		memset(((uint8_t *)pcm_buf) + size, 0, USB_STEREO_FRAME_SIZE - size);
	}

	if (CONFIG_INFO_REPORTING_INTERVAL > 0) {
		if (size != 0U) {
			static size_t cnt;

			if (++cnt % (CONFIG_INFO_REPORTING_INTERVAL * 10) == 0U) {
				LOG_INF("[%zu]: Sending USB audio", cnt);
			}
		} else {
			static size_t cnt;

			if (++cnt % (CONFIG_INFO_REPORTING_INTERVAL * 10) == 0U) {
				LOG_INF("[%zu]: Sending empty USB audio", cnt);
			}
		}
	}

	err = usbd_uac2_send(dev, IN_TERMINAL_ID, pcm_buf, USB_STEREO_FRAME_SIZE);
	if (err != 0) {
		if (CONFIG_INFO_REPORTING_INTERVAL > 0) {
			static size_t cnt;

			if (cnt++ % (CONFIG_INFO_REPORTING_INTERVAL * 10) == 0) {
				LOG_ERR("[%zu]: Failed to send USB audio: %d", cnt, err);
			}
		}

		k_mem_slab_free(&usb_in_buf_pool, pcm_buf);
	} /* USB owns the buffer which will be released in uac2_buf_release_cb */
}

static void uac2_buf_release_cb(const struct device *dev, uint8_t terminal, void *buf,
				void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(user_data);

	k_mem_slab_free(&usb_in_buf_pool, buf);
}

static void terminal_update_cb(const struct device *dev, uint8_t terminal, bool enabled,
			       bool microframes, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(terminal);
	ARG_UNUSED(microframes);
	ARG_UNUSED(user_data);

	terminal_enabled = enabled;
}

int usb_add_frame_to_usb(enum bt_audio_location chan_allocation, const int16_t *frame,
			 size_t frame_size, uint32_t ts)
{
	static size_t cnt;

	if (!terminal_enabled) {
		/* Simply discard the data then */
		/* TODO: Consider if we still want to decode the incoming audio */
		return 0;
	}

	if (CONFIG_INFO_REPORTING_INTERVAL > 0 && (++cnt % CONFIG_INFO_REPORTING_INTERVAL) == 0U) {
		LOG_INF("[%zu]: Adding USB audio frame", cnt);
	}

	return stereo_out_add_frame(chan_allocation, frame, frame_size, ts);
}

int usb_init(void)
{
	const struct device *mic_dev = DEVICE_DT_GET(DT_NODELABEL(uac2_microphone));
	static struct uac2_ops usb_audio_ops = {
		.sof_cb = uac2_sof_cb,
		.buf_release_cb = uac2_buf_release_cb,
		.terminal_update_cb = terminal_update_cb,
	};
	struct usbd_context *sample_usbd;
	static bool initialized;
	int err;

	if (initialized) {
		return -EALREADY;
	}

	if (!device_is_ready(mic_dev)) {
		LOG_ERR("Cannot get USB Microphone Device");
		return -EIO;
	}

	usbd_uac2_set_ops(mic_dev, &usb_audio_ops, NULL);

	sample_usbd = sample_usbd_init_device(NULL);
	if (sample_usbd == NULL) {
		return -ENODEV;
	}

	err = usbd_enable(sample_usbd);
	if (err != 0) {
		return err;
	}

	LOG_INF("USB initialized");
	initialized = true;

	return 0;
}

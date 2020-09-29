/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for Audio class
 */

#include <zephyr.h>
#include <logging/log.h>
#include <usb/usb_device.h>
#include <usb/class/usb_audio.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct device *mic_dev;

static void data_received(const struct device *dev,
			  struct net_buf *buffer,
			  size_t size)
{
	int ret;

	if (!buffer || !size) {
		/* This should never happen */
		return;
	}

	LOG_DBG("Received %d data, buffer %p", size, buffer);

	/* Check if OUT device buffer can be used for IN device */
	if (size == usb_audio_get_in_frame_size(mic_dev)) {
		ret = usb_audio_send(mic_dev, buffer, size);
		if (ret) {
			net_buf_unref(buffer);
		}
	} else {
		net_buf_unref(buffer);
	}
}

static void feature_update(const struct device *dev,
			   const struct usb_audio_fu_evt *evt)
{
	LOG_DBG("Control selector %d for channel %d updated",
		evt->cs, evt->channel);
	switch (evt->cs) {
	case USB_AUDIO_FU_MUTE_CONTROL:
	default:
		break;
	}
}

static const struct usb_audio_ops hp_ops = {
	.data_received_cb = data_received,
	.feature_update_cb = feature_update,
};

static const struct usb_audio_ops mic_ops = {
	.feature_update_cb = feature_update,
};

void main(void)
{
	const struct device *hp_dev = device_get_binding("HEADPHONES");
	int ret;

	LOG_INF("Entered %s", __func__);
	mic_dev = device_get_binding("MICROPHONE");

	if (!hp_dev) {
		LOG_ERR("Can not get USB Headphones Device");
		return;
	}

	if (!mic_dev) {
		LOG_ERR("Can not get USB Microphone Device");
		return;
	}

	usb_audio_register(hp_dev, &hp_ops);

	usb_audio_register(mic_dev, &mic_ops);

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}
}
